#include "metrics/cpu_usage_calculator.hpp"
#include "parsers/proc_stat_parser.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <limits>
#include <string>

namespace
{

tsm::CpuSample LoadSample(const std::string& name)
{
    std::ifstream input(
            std::string(TsmTestFixtureDir) + "/proc_stat/" + name);
    REQUIRE(input);

    auto parsed = tsm::ProcStatParser{}.Parse(input);
    REQUIRE(parsed);
    return parsed.Value();
}

}  // namespace

TEST_CASE("CPU usage is calculated from two consecutive samples")
{
    const auto previous = LoadSample("sample_a.txt");
    const auto current = LoadSample("sample_b.txt");

    const auto result =
        tsm::CpuUsageCalculator{}.Calculate(previous, current);
    REQUIRE(result);
    REQUIRE(result.Value().aggregatePercent);
    CHECK(*result.Value().aggregatePercent ==
            Catch::Approx(56.25));
    REQUIRE(result.Value().corePercentages.size() == 2);
    REQUIRE(result.Value().corePercentages[0]);
    CHECK(*result.Value().corePercentages[0] ==
            Catch::Approx(56.25));
}

TEST_CASE("A newly appeared CPU core has unknown usage")
{
    auto previous = LoadSample("sample_a.txt");
    auto current = LoadSample("sample_b.txt");
    current.cores.push_back({"cpu2", {1, 0, 0, 10, 0, 0, 0, 0}});

    const auto result =
        tsm::CpuUsageCalculator{}.Calculate(previous, current);
    REQUIRE(result);
    REQUIRE(result.Value().corePercentages.size() == 3);
    CHECK_FALSE(result.Value().corePercentages[2]);
}

TEST_CASE("Zero, reset, and overflowing aggregate counters are rejected")
{
    const auto sample = LoadSample("sample_a.txt");
    CHECK_FALSE(tsm::CpuUsageCalculator{}.Calculate(sample, sample));

    auto reset = LoadSample("sample_b.txt");
    reset.aggregate.user = 1;
    reset.aggregate.idle = 1;
    CHECK_FALSE(tsm::CpuUsageCalculator{}.Calculate(sample, reset));

    auto overflowing = LoadSample("sample_b.txt");
    overflowing.aggregate.user =
        std::numeric_limits<std::uint64_t>::max();
    CHECK_FALSE(
            tsm::CpuUsageCalculator{}.Calculate(sample, overflowing));
}
