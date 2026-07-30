#include "metrics/cpu_usage_calculator.hpp"
#include "parsers/proc_stat_parser.hpp"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace {

std::ifstream fixture(const std::string& name) {
    return std::ifstream(
        std::string(TSM_TEST_FIXTURE_DIR) + "/proc_stat/" + name);
}

}  // namespace

TEST_CASE("ProcStatParser reads aggregate and per-core counters") {
    auto input = fixture("sample_a.txt");
    REQUIRE(input);

    const auto result = tsm::ProcStatParser{}.parse(input);
    REQUIRE(result);
    CHECK(result.value().aggregate.user == 100);
    CHECK(result.value().aggregate.io_wait == 10);
    REQUIRE(result.value().cores.size() == 2);
    CHECK(result.value().cores[0].name == "cpu0");
    CHECK(result.value().cores[1].times.soft_irq == 2);
}

TEST_CASE("ProcStatParser accepts prepared text without filesystem access") {
    const std::string text =
        "cpu 1 2 3 4\n"
        "cpu0 1 1 1 1\n"
        "processes 42\n";

    const auto result = tsm::ProcStatParser{}.parse(text);
    REQUIRE(result);
    CHECK(result.value().aggregate.steal == 0);
    REQUIRE(result.value().cores.size() == 1);
}

TEST_CASE("ProcStatParser rejects malformed and incomplete input") {
    auto malformed = fixture("malformed.txt");
    REQUIRE(malformed);
    const auto malformed_result =
        tsm::ProcStatParser{}.parse(malformed);
    REQUIRE_FALSE(malformed_result);
    CHECK(malformed_result.error().kind == tsm::ErrorKind::parse);

    std::istringstream missing_aggregate("cpu0 1 2 3 4\n");
    const auto missing_result =
        tsm::ProcStatParser{}.parse(missing_aggregate);
    REQUIRE_FALSE(missing_result);
    CHECK(missing_result.error().context == "/proc/stat");
}

TEST_CASE("The running Linux system exposes parseable CPU counters",
          "[integration][linux]") {
    std::ifstream first_input("/proc/stat");
    REQUIRE(first_input);
    const auto first = tsm::ProcStatParser{}.parse(first_input);
    REQUIRE(first);

    std::this_thread::sleep_for(std::chrono::milliseconds(25));

    std::ifstream second_input("/proc/stat");
    REQUIRE(second_input);
    const auto second = tsm::ProcStatParser{}.parse(second_input);
    REQUIRE(second);

    CHECK(second.value().aggregate.user >= first.value().aggregate.user);
    CHECK(second.value().aggregate.idle >= first.value().aggregate.idle);
    CHECK_FALSE(second.value().cores.empty());

    const auto usage =
        tsm::CpuUsageCalculator{}.calculate(first.value(), second.value());
    REQUIRE(usage);
    REQUIRE(usage.value().aggregate_percent);
    CHECK(*usage.value().aggregate_percent >= 0.0);
    CHECK(*usage.value().aggregate_percent <= 100.0);
}
