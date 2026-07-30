#include "metrics/cpu_usage_calculator.hpp"
#include "parsers/proc_stat_parser.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <limits>
#include <string>

namespace {

tsm::CpuSample load_sample(const std::string& name) {
    std::ifstream input(
        std::string(TSM_TEST_FIXTURE_DIR) + "/proc_stat/" + name);
    REQUIRE(input);

    auto parsed = tsm::ProcStatParser{}.parse(input);
    REQUIRE(parsed);
    return parsed.value();
}

}  // namespace

TEST_CASE("CPU usage is calculated from two consecutive samples") {
    const auto previous = load_sample("sample_a.txt");
    const auto current = load_sample("sample_b.txt");

    const auto result =
        tsm::CpuUsageCalculator{}.calculate(previous, current);
    REQUIRE(result);
    REQUIRE(result.value().aggregate_percent);
    CHECK(*result.value().aggregate_percent ==
          Catch::Approx(56.25));
    REQUIRE(result.value().core_percentages.size() == 2);
    REQUIRE(result.value().core_percentages[0]);
    CHECK(*result.value().core_percentages[0] ==
          Catch::Approx(56.25));
}

TEST_CASE("A newly appeared CPU core has unknown usage") {
    auto previous = load_sample("sample_a.txt");
    auto current = load_sample("sample_b.txt");
    current.cores.push_back({"cpu2", {1, 0, 0, 10, 0, 0, 0, 0}});

    const auto result =
        tsm::CpuUsageCalculator{}.calculate(previous, current);
    REQUIRE(result);
    REQUIRE(result.value().core_percentages.size() == 3);
    CHECK_FALSE(result.value().core_percentages[2]);
}

TEST_CASE("Zero, reset, and overflowing aggregate counters are rejected") {
    const auto sample = load_sample("sample_a.txt");
    CHECK_FALSE(tsm::CpuUsageCalculator{}.calculate(sample, sample));

    auto reset = load_sample("sample_b.txt");
    reset.aggregate.user = 1;
    reset.aggregate.idle = 1;
    CHECK_FALSE(tsm::CpuUsageCalculator{}.calculate(sample, reset));

    auto overflowing = load_sample("sample_b.txt");
    overflowing.aggregate.user =
        std::numeric_limits<std::uint64_t>::max();
    CHECK_FALSE(
        tsm::CpuUsageCalculator{}.calculate(sample, overflowing));
}
