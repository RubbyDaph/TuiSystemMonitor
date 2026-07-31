#include "collectors/process_collector.hpp"
#include "linux/linux_proc_source.hpp"
#include "linux/linux_user_resolver.hpp"
#include "metrics/process_sorter.hpp"
#include "metrics/process_usage_calculator.hpp"
#include "parsers/proc_stat_parser.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace
{

tsm::CpuSample MakeCpuSample(
    std::uint64_t user,
    std::uint64_t idle,
    std::size_t coreCount)
{
    tsm::CpuSample sample;
    sample.aggregate.user = user;
    sample.aggregate.idle = idle;
    for (std::size_t index = 0; index < coreCount; ++index)
    {
        sample.cores.push_back(
            {"cpu" + std::to_string(index), {}});
    }
    return sample;
}

tsm::ProcessRawSample MakeProcess(
    tsm::ProcessId pid,
    std::uint64_t startTime,
    std::uint64_t userTicks,
    std::uint64_t systemTicks,
    std::uint64_t memory)
{
    return {
        {pid, startTime},
        "process" + std::to_string(pid),
        1000,
        "alice",
        tsm::ProcessState::Running,
        userTicks,
        systemTicks,
        memory,
    };
}

}  // namespace

TEST_CASE("Process CPU uses one logical core as one hundred percent")
{
    const auto previousCpu = MakeCpuSample(100, 900, 4);
    const auto currentCpu = MakeCpuSample(140, 940, 4);
    const std::vector<tsm::ProcessRawSample> previous{
        MakeProcess(10, 1000, 10, 5, 1024),
        MakeProcess(20, 2000, 20, 10, 2048),
    };
    const std::vector<tsm::ProcessRawSample> current{
        MakeProcess(10, 1000, 18, 7, 4096),
        MakeProcess(20, 2000, 50, 20, 8192),
    };

    const auto result = tsm::ProcessUsageCalculator{}.Calculate(
        previous, current, previousCpu, currentCpu);

    REQUIRE(result);
    REQUIRE(result.Value().size() == 2);
    REQUIRE(result.Value()[0].cpuPercent);
    REQUIRE(result.Value()[1].cpuPercent);
    CHECK(*result.Value()[0].cpuPercent == Catch::Approx(50.0));
    CHECK(*result.Value()[1].cpuPercent == Catch::Approx(200.0));
    CHECK(result.Value()[1].residentMemoryBytes == 8192);
    CHECK(result.Value()[1].user == "alice");
}

TEST_CASE("New and reused process identities have unknown CPU usage")
{
    const auto previousCpu = MakeCpuSample(100, 900, 2);
    const auto currentCpu = MakeCpuSample(120, 920, 2);
    const std::vector<tsm::ProcessRawSample> previous{
        MakeProcess(10, 1000, 10, 0, 100),
    };
    const std::vector<tsm::ProcessRawSample> current{
        MakeProcess(10, 2000, 30, 0, 200),
        MakeProcess(11, 3000, 5, 0, 300),
    };

    const auto result = tsm::ProcessUsageCalculator{}.Calculate(
        previous, current, previousCpu, currentCpu);

    REQUIRE(result);
    REQUIRE(result.Value().size() == 2);
    CHECK_FALSE(result.Value()[0].cpuPercent);
    CHECK_FALSE(result.Value()[1].cpuPercent);
}

TEST_CASE("Reset and overflowing process counters produce unknown CPU")
{
    const auto previousCpu = MakeCpuSample(100, 900, 1);
    const auto currentCpu = MakeCpuSample(110, 910, 1);
    const std::vector<tsm::ProcessRawSample> previous{
        MakeProcess(10, 1000, 100, 50, 100),
        MakeProcess(
            20,
            2000,
            std::numeric_limits<std::uint64_t>::max(),
            1,
            200),
    };
    const std::vector<tsm::ProcessRawSample> current{
        MakeProcess(10, 1000, 1, 1, 100),
        MakeProcess(
            20,
            2000,
            std::numeric_limits<std::uint64_t>::max(),
            2,
            200),
    };

    const auto result = tsm::ProcessUsageCalculator{}.Calculate(
        previous, current, previousCpu, currentCpu);

    REQUIRE(result);
    CHECK_FALSE(result.Value()[0].cpuPercent);
    CHECK_FALSE(result.Value()[1].cpuPercent);
}

TEST_CASE("Invalid aggregate CPU delta rejects process calculation")
{
    const auto cpu = MakeCpuSample(100, 900, 2);
    const std::vector<tsm::ProcessRawSample> processes{
        MakeProcess(10, 1000, 10, 0, 100),
    };

    CHECK_FALSE(tsm::ProcessUsageCalculator{}.Calculate(
        processes, processes, cpu, cpu));

    auto overflowing = cpu;
    overflowing.aggregate.user =
        std::numeric_limits<std::uint64_t>::max();
    CHECK_FALSE(tsm::ProcessUsageCalculator{}.Calculate(
        processes, processes, cpu, overflowing));
}

TEST_CASE("Processes sort deterministically by name CPU and memory")
{
    std::vector<tsm::ProcessInfo> processes{
        {{30, 1}, "third", "alice", tsm::ProcessState::Sleeping, 50.0, 300},
        {{10, 1}, "first", "alice", tsm::ProcessState::Running, 80.0, 100},
        {{20, 1}, "second", "alice", tsm::ProcessState::Running, 80.0, 500},
        {{40, 1}, "new", "alice", tsm::ProcessState::Running, std::nullopt, 200},
    };

    tsm::ProcessSorter{}.Sort(processes, tsm::ProcessSortKey::Name);
    CHECK(processes[0].identity.pid == 10);
    CHECK(processes[1].identity.pid == 40);
    CHECK(processes[3].identity.pid == 30);

    tsm::ProcessSorter{}.Sort(processes, tsm::ProcessSortKey::Cpu);
    CHECK(processes[0].identity.pid == 10);
    CHECK(processes[1].identity.pid == 20);
    CHECK(processes[3].identity.pid == 40);

    tsm::ProcessSorter{}.Sort(processes, tsm::ProcessSortKey::Memory);
    CHECK(processes[0].identity.pid == 20);
    CHECK(processes[3].identity.pid == 10);
}

TEST_CASE("Two real Linux samples produce process statistics",
          "[integration][linux]")
{
    const tsm::LinuxProcSource source;
    const tsm::LinuxUserResolver users;
    const tsm::ProcessCollector collector(source, users);
    const tsm::ProcStatParser cpuParser;

    std::ifstream firstCpuInput("/proc/stat");
    REQUIRE(firstCpuInput);
    const auto firstCpu = cpuParser.Parse(firstCpuInput);
    const auto firstProcesses = collector.Collect();
    REQUIRE(firstCpu);

    std::this_thread::sleep_for(std::chrono::milliseconds(25));

    std::ifstream secondCpuInput("/proc/stat");
    REQUIRE(secondCpuInput);
    const auto secondCpu = cpuParser.Parse(secondCpuInput);
    const auto secondProcesses = collector.Collect();
    REQUIRE(secondCpu);

    const auto result = tsm::ProcessUsageCalculator{}.Calculate(
        firstProcesses.processes,
        secondProcesses.processes,
        firstCpu.Value(),
        secondCpu.Value());
    REQUIRE(result);
    REQUIRE_FALSE(result.Value().empty());

    const auto currentPid = static_cast<tsm::ProcessId>(getpid());
    const auto self = std::find_if(
        result.Value().begin(),
        result.Value().end(),
        [currentPid](const tsm::ProcessInfo& process)
        {
            return process.identity.pid == currentPid;
        });
    CHECK(self != result.Value().end());
}
