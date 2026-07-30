#include "domain/process.hpp"
#include "domain/result.hpp"
#include "domain/snapshot.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("A process identity includes both PID and start time") {
    const tsm::ProcessIdentity original{42, 1000};

    CHECK(original == tsm::ProcessIdentity{42, 1000});
    CHECK(original != tsm::ProcessIdentity{42, 1001});
    CHECK(original != tsm::ProcessIdentity{43, 1000});
}

TEST_CASE("Result represents either a value or a structured error") {
    auto success = tsm::Result<int>::success(17);
    REQUIRE(success);
    CHECK(success.value() == 17);

    auto failure = tsm::Result<int>::failure(
        {tsm::ErrorKind::parse, "/proc/stat", {}, "invalid CPU line"});
    REQUIRE_FALSE(failure);
    CHECK(failure.error().kind == tsm::ErrorKind::parse);
    CHECK(failure.error().context == "/proc/stat");
}

TEST_CASE("A first snapshot can represent unknown CPU usage") {
    tsm::ApplicationSnapshot snapshot;
    snapshot.system.cpu.data = tsm::CpuUsage{};

    REQUIRE(snapshot.system.cpu.data);
    CHECK_FALSE(snapshot.system.cpu.data->aggregate_percent);
    CHECK(snapshot.processes.processes.empty());
}
