#include "parsers/meminfo_parser.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>

namespace
{

constexpr std::uint64_t kib = 1024;

std::ifstream Fixture(const std::string& name)
{
    return std::ifstream(
            std::string(TsmTestFixtureDir) + "/meminfo/" + name);
}

}  // namespace

TEST_CASE("MeminfoParser calculates RAM and swap usage")
{
    auto input = Fixture("sample.txt");
    REQUIRE(input);

    const auto result = tsm::MeminfoParser{}.Parse(input);
    REQUIRE(result);
    CHECK(result.Value().ramTotalBytes == 16384000 * kib);
    CHECK(result.Value().ramAvailableBytes == 8192000 * kib);
    CHECK(result.Value().ramUsedBytes == 8192000 * kib);
    CHECK(result.Value().swapTotalBytes == 2097152 * kib);
    CHECK(result.Value().swapFreeBytes == 1048576 * kib);
    CHECK(result.Value().swapUsedBytes == 1048576 * kib);
}

TEST_CASE("MeminfoParser supports a system without swap")
{
    auto input = Fixture("no_swap.txt");
    REQUIRE(input);

    const auto result = tsm::MeminfoParser{}.Parse(input);
    REQUIRE(result);
    CHECK(result.Value().swapTotalBytes == 0);
    CHECK(result.Value().swapUsedBytes == 0);
    CHECK(result.Value().swapFreeBytes == 0);
}

TEST_CASE("MeminfoParser calculates a fallback without MemAvailable")
{
    auto input = Fixture("legacy.txt");
    REQUIRE(input);

    const auto result = tsm::MeminfoParser{}.Parse(input);
    REQUIRE(result);
    CHECK(result.Value().ramAvailableBytes == 4500 * kib);
    CHECK(result.Value().ramUsedBytes == 5500 * kib);
}

TEST_CASE("MeminfoParser accepts prepared text")
{
    const std::string text =
        "MemTotal: 100 kB\n"
        "MemAvailable: 40 kB\n"
        "SwapTotal: 20 kB\n"
        "SwapFree: 5 kB\n";

    const auto result = tsm::MeminfoParser{}.Parse(text);
    REQUIRE(result);
    CHECK(result.Value().ramUsedBytes == 60 * kib);
    CHECK(result.Value().swapUsedBytes == 15 * kib);
}

TEST_CASE("MeminfoParser rejects malformed, missing, and invalid data")
{
    auto malformed = Fixture("malformed.txt");
    REQUIRE(malformed);
    CHECK_FALSE(tsm::MeminfoParser{}.Parse(malformed));

    std::istringstream missingSwap(
            "MemTotal: 100 kB\n"
            "MemAvailable: 50 kB\n");
    CHECK_FALSE(tsm::MeminfoParser{}.Parse(missingSwap));

    const std::string impossible =
        "MemTotal: 100 kB\n"
        "MemAvailable: 101 kB\n"
        "SwapTotal: 10 kB\n"
        "SwapFree: 11 kB\n";
    CHECK_FALSE(tsm::MeminfoParser{}.Parse(impossible));
}

TEST_CASE("MeminfoParser rejects byte conversion overflow")
{
    const std::string overflowing =
        "MemTotal: 18014398509481984 kB\n"
        "MemAvailable: 1 kB\n"
        "SwapTotal: 0 kB\n"
        "SwapFree: 0 kB\n";

    const auto result = tsm::MeminfoParser{}.Parse(overflowing);
    REQUIRE_FALSE(result);
    CHECK(result.GetError().kind == tsm::ErrorKind::InvalidData);
}

TEST_CASE("The running Linux system exposes valid memory data",
        "[integration][linux]")
{
    std::ifstream input("/proc/meminfo");
    REQUIRE(input);

    const auto result = tsm::MeminfoParser{}.Parse(input);
    REQUIRE(result);
    CHECK(result.Value().ramTotalBytes > 0);
    CHECK(result.Value().ramUsedBytes <=
            result.Value().ramTotalBytes);
    CHECK(result.Value().ramAvailableBytes <=
            result.Value().ramTotalBytes);
    CHECK(result.Value().swapUsedBytes +
            result.Value().swapFreeBytes ==
            result.Value().swapTotalBytes);
}
