#include "app/app_state.hpp"
#include "app/command_controller.hpp"
#include "collectors/filesystem_collector.hpp"
#include "collectors/process_collector.hpp"
#include "collectors/snapshot_collector.hpp"
#include "linux/linux_filesystem_stats.hpp"
#include "linux/linux_proc_source.hpp"
#include "linux/linux_system_source.hpp"
#include "linux/linux_user_resolver.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

namespace
{

std::string FixtureText(
    const std::string& directory,
    const std::string& name)
{
    std::ifstream input(
        std::string(TsmTestFixtureDir) + "/" + directory + "/" + name);
    REQUIRE(input);
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
}

std::string ProcessStat(
    std::uint64_t userTicks,
    std::uint64_t systemTicks)
{
    return "123 (worker) R 1 2 3 0 -1 4194304 10 0 1 0 " +
           std::to_string(userTicks) + " " +
           std::to_string(systemTicks) +
           " 0 0 20 0 4 0 123456 4096 10\n";
}

class FakeSystemSource final : public tsm::SystemSource
{
public:
    tsm::Result<std::string> ReadCpuStat() const override
    {
        return tsm::Result<std::string>::Success(
            FixtureText(
                "proc_stat",
                index == 0 ? "sample_a.txt" : "sample_b.txt"));
    }

    tsm::Result<std::string> ReadMeminfo() const override
    {
        return tsm::Result<std::string>::Success(
            FixtureText("meminfo", "sample.txt"));
    }

    tsm::Result<std::string> ReadMountinfo() const override
    {
        auto result = tsm::Result<std::string>::Success(
            FixtureText("mountinfo", "sample.txt"));
        if (index == 0)
        {
            index = 1;
        }
        return result;
    }

private:
    mutable int index{};
};

class FakeFilesystemStats final : public tsm::FilesystemStatsSource
{
public:
    tsm::Result<tsm::FilesystemUsage> Read(
        const tsm::MountInfo& mount) const override
    {
        return tsm::Result<tsm::FilesystemUsage>::Success(
            {mount, 1000, 400, 500, 44.444, mount.readOnly});
    }
};

class FakeProcSource final : public tsm::ProcSource
{
public:
    tsm::Result<std::vector<tsm::ProcessId>>
    ListProcessIds() const override
    {
        activeIndex = nextIndex;
        nextIndex = 1;
        return tsm::Result<std::vector<tsm::ProcessId>>::Success(
            {123});
    }

    tsm::Result<std::string> ReadProcessStat(
        tsm::ProcessId) const override
    {
        return tsm::Result<std::string>::Success(
            activeIndex == 0
                ? ProcessStat(100, 50)
                : ProcessStat(115, 55));
    }

    tsm::Result<std::string> ReadProcessStatus(
        tsm::ProcessId) const override
    {
        return tsm::Result<std::string>::Success(
            FixtureText("process", "status.txt"));
    }

private:
    mutable int activeIndex{};
    mutable int nextIndex{};
};

class FakeUserResolver final : public tsm::UserResolver
{
public:
    tsm::Result<std::string> Resolve(
        std::uint32_t) const override
    {
        return tsm::Result<std::string>::Success("alice");
    }
};

struct TestCollectors
{
    FakeSystemSource systemSource;
    FakeFilesystemStats filesystemStats;
    tsm::FilesystemCollector filesystemCollector{filesystemStats};
    FakeProcSource procSource;
    FakeUserResolver userResolver;
    tsm::ProcessCollector processCollector{procSource, userResolver};
    tsm::SnapshotCollector snapshotCollector{
        systemSource,
        filesystemCollector,
        processCollector};
};

}  // namespace

TEST_CASE("SnapshotCollector combines all metric subsystems")
{
    TestCollectors collectors;

    const auto first = collectors.snapshotCollector.Collect();
    REQUIRE(first.system.cpu.data);
    CHECK_FALSE(first.system.cpu.data->aggregatePercent);
    REQUIRE(first.system.memory.data);
    REQUIRE(first.system.filesystems.data);
    REQUIRE(first.processes.processes.size() == 1);
    CHECK_FALSE(first.processes.processes[0].cpuPercent);

    const auto second = collectors.snapshotCollector.Collect();
    REQUIRE(second.system.cpu.data);
    REQUIRE(second.system.cpu.data->aggregatePercent);
    REQUIRE(second.processes.processes.size() == 1);
    REQUIRE(second.processes.processes[0].cpuPercent);
    CHECK(second.processes.processes[0].user == "alice");
    CHECK(second.processes.processes[0].residentMemoryBytes ==
          2048 * 1024);
}

TEST_CASE("AppState keeps last good sections after partial failure")
{
    TestCollectors collectors;
    tsm::AppState state;
    state.ApplySnapshot(collectors.snapshotCollector.Collect());

    REQUIRE(state.Snapshot());
    REQUIRE(state.Snapshot()->system.memory.data);
    REQUIRE(state.Snapshot()->processes.processes.size() == 1);
    const auto selected = state.SelectedProcess();
    REQUIRE(selected);

    tsm::ApplicationSnapshot failed;
    failed.collectedAt = std::chrono::steady_clock::now();
    failed.system.memory.errors.push_back(
        {tsm::ErrorKind::Io, "/proc/meminfo", {}, "read failed"});
    failed.processes.available = false;
    failed.processes.errors.push_back(
        {tsm::ErrorKind::Io, "/proc", {}, "read failed"});
    state.ApplySnapshot(std::move(failed));

    REQUIRE(state.Snapshot()->system.memory.data);
    CHECK(state.Snapshot()->system.memory.stale);
    REQUIRE(state.Snapshot()->processes.processes.size() == 1);
    CHECK(state.Snapshot()->processes.stale);
    CHECK(state.SelectedProcess() == selected);
    CHECK(state.StatusMessage() == "Updated with errors");
}

TEST_CASE("AppState sorts processes and restores selection")
{
    tsm::ApplicationSnapshot snapshot;
    snapshot.processes.processes = {
        {{20, 1}, "second", "alice", tsm::ProcessState::Running, 10.0, 500},
        {{10, 1}, "first", "alice", tsm::ProcessState::Running, 20.0, 100},
    };

    tsm::AppState state;
    state.ApplySnapshot(std::move(snapshot));
    REQUIRE(state.SelectedProcess());
    CHECK(state.SelectedProcess()->pid == 10);

    state.SetSelectedProcess(tsm::ProcessIdentity{20, 1});
    state.SetSortKey(tsm::ProcessSortKey::Memory);
    REQUIRE(state.SelectedProcess());
    CHECK(state.SelectedProcess()->pid == 20);
    REQUIRE(state.Snapshot());
    CHECK(state.Snapshot()->processes.processes[0].identity.pid == 20);
}

TEST_CASE("CommandController performs a complete manual refresh")
{
    TestCollectors collectors;
    tsm::AppState state;
    tsm::CommandController controller(
        state, collectors.snapshotCollector);

    controller.Refresh();

    CHECK_FALSE(state.IsRefreshing());
    REQUIRE(state.Snapshot());
    CHECK(state.StatusMessage() == "Updated successfully");
}

TEST_CASE("Real Linux sources produce an application snapshot",
          "[integration][linux]")
{
    const tsm::LinuxSystemSource systemSource;
    const tsm::LinuxFilesystemStats filesystemStats;
    const tsm::FilesystemCollector filesystemCollector(
        filesystemStats);
    const tsm::LinuxProcSource procSource;
    const tsm::LinuxUserResolver userResolver;
    const tsm::ProcessCollector processCollector(
        procSource, userResolver);
    tsm::SnapshotCollector collector(
        systemSource, filesystemCollector, processCollector);

    const auto first = collector.Collect();
    REQUIRE(first.system.memory.data);
    REQUIRE(first.system.filesystems.data);
    REQUIRE_FALSE(first.processes.processes.empty());

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    const auto second = collector.Collect();
    REQUIRE(second.system.cpu.data);
    REQUIRE(second.system.cpu.data->aggregatePercent);
    REQUIRE_FALSE(second.processes.processes.empty());
}
