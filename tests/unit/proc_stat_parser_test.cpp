#include "metrics/cpu_usage_calculator.hpp"
#include "parsers/proc_stat_parser.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace
{

std::ifstream Fixture(const std::string& name)
{
    return std::ifstream(
            std::string(TsmTestFixtureDir) + "/proc_stat/" + name);
}

}  // namespace

TEST_CASE("ProcStatParser reads aggregate and per-core counters")
{
    auto input = Fixture("sample_a.txt");
    REQUIRE(input);

    const auto result = tsm::ProcStatParser{}.Parse(input);
    REQUIRE(result);
    CHECK(result.Value().aggregate.user == 100);
    CHECK(result.Value().aggregate.ioWait == 10);
    REQUIRE(result.Value().cores.size() == 2);
    CHECK(result.Value().cores[0].name == "cpu0");
    CHECK(result.Value().cores[1].times.softIrq == 2);
}

TEST_CASE("ProcStatParser accepts prepared text without filesystem access")
{
    const std::string text =
        "cpu 1 2 3 4\n"
        "cpu0 1 1 1 1\n"
        "processes 42\n";

    const auto result = tsm::ProcStatParser{}.Parse(text);
    REQUIRE(result);
    CHECK(result.Value().aggregate.steal == 0);
    REQUIRE(result.Value().cores.size() == 1);
}

TEST_CASE("ProcStatParser rejects malformed and incomplete input")
{
    auto malformed = Fixture("malformed.txt");
    REQUIRE(malformed);
    const auto malformedResult =
        tsm::ProcStatParser{}.Parse(malformed);
    REQUIRE_FALSE(malformedResult);
    CHECK(malformedResult.GetError().kind == tsm::ErrorKind::Parse);

    std::istringstream missingAggregate("cpu0 1 2 3 4\n");
    const auto missingResult =
        tsm::ProcStatParser{}.Parse(missingAggregate);
    REQUIRE_FALSE(missingResult);
    CHECK(missingResult.GetError().context == "/proc/stat");
}

TEST_CASE("The running Linux system exposes parseable CPU counters",
        "[integration][linux]")
{
    std::ifstream firstInput("/proc/stat");
    REQUIRE(firstInput);
    const auto first = tsm::ProcStatParser{}.Parse(firstInput);
    REQUIRE(first);

    std::this_thread::sleep_for(std::chrono::milliseconds(25));

    std::ifstream secondInput("/proc/stat");
    REQUIRE(secondInput);
    const auto second = tsm::ProcStatParser{}.Parse(secondInput);
    REQUIRE(second);

    CHECK(second.Value().aggregate.user >= first.Value().aggregate.user);
    CHECK(second.Value().aggregate.idle >= first.Value().aggregate.idle);
    CHECK_FALSE(second.Value().cores.empty());

    const auto usage =
        tsm::CpuUsageCalculator{}.Calculate(first.Value(), second.Value());
    REQUIRE(usage);
    REQUIRE(usage.Value().aggregatePercent);
    CHECK(*usage.Value().aggregatePercent >= 0.0);
    CHECK(*usage.Value().aggregatePercent <= 100.0);
}
