#include "collectors/filesystem_collector.hpp"
#include "linux/linux_filesystem_stats.hpp"
#include "metrics/filesystem_usage_calculator.hpp"
#include "parsers/mountinfo_parser.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

namespace
{

std::ifstream MountFixture(const std::string& name)
{
    return std::ifstream(
        std::string(TsmTestFixtureDir) + "/mountinfo/" + name);
}

class FakeFilesystemStats final : public tsm::FilesystemStatsSource
{
public:
    tsm::Result<tsm::FilesystemUsage> Read(
        const tsm::MountInfo& mount) const override
    {
        if (mount.mountPoint == "/srv/data")
        {
            return tsm::Result<tsm::FilesystemUsage>::Failure(
                {tsm::ErrorKind::Disappeared,
                 mount.mountPoint,
                 {},
                 "mount disappeared"});
        }

        return tsm::FilesystemUsageCalculator{}.Calculate(
            mount, 100, 40, 30, 1024, false);
    }
};

}  // namespace

TEST_CASE("MountinfoParser reads and decodes mounted filesystems")
{
    auto input = MountFixture("sample.txt");
    REQUIRE(input);

    const auto result = tsm::MountinfoParser{}.Parse(input);
    REQUIRE(result);
    REQUIRE(result.Value().size() == 5);
    CHECK(result.Value()[0].mountPoint == "/");
    CHECK(result.Value()[3].mountPoint == "/media/My Disk");
    CHECK(result.Value()[4].source == "server:/exports/data");
    CHECK(result.Value()[4].readOnly);
}

TEST_CASE("MountinfoParser accepts text and rejects invalid layouts")
{
    const std::string text =
        "1 0 8:1 / / rw - ext4 /dev/root rw\n";
    const auto valid = tsm::MountinfoParser{}.Parse(text);
    REQUIRE(valid);
    REQUIRE(valid.Value().size() == 1);

    auto malformed = MountFixture("malformed.txt");
    REQUIRE(malformed);
    CHECK_FALSE(tsm::MountinfoParser{}.Parse(malformed));

    const std::string invalidEscape =
        "1 0 8:1 / /bad\\09x rw - ext4 /dev/root rw\n";
    CHECK_FALSE(tsm::MountinfoParser{}.Parse(invalidEscape));
}

TEST_CASE("Filesystem usage follows df block semantics")
{
    const tsm::MountInfo mount{"/dev/sda1", "/", "ext4", false};
    const auto result = tsm::FilesystemUsageCalculator{}.Calculate(
        mount, 100, 40, 30, 1024, false);

    REQUIRE(result);
    CHECK(result.Value().totalBytes == 102400);
    CHECK(result.Value().usedBytes == 61440);
    CHECK(result.Value().availableBytes == 30720);
    CHECK(result.Value().usedPercent ==
          Catch::Approx(66.666666).epsilon(0.001));
}

TEST_CASE("Filesystem usage rejects invalid and overflowing counters")
{
    const tsm::MountInfo mount{"/dev/sda1", "/", "ext4", false};
    CHECK_FALSE(tsm::FilesystemUsageCalculator{}.Calculate(
        mount, 10, 11, 0, 1024, false));
    CHECK_FALSE(tsm::FilesystemUsageCalculator{}.Calculate(
        mount, 10, 5, 6, 1024, false));
    CHECK_FALSE(tsm::FilesystemUsageCalculator{}.Calculate(
        mount, 10, 5, 4, 0, false));
    CHECK_FALSE(tsm::FilesystemUsageCalculator{}.Calculate(
        mount,
        std::numeric_limits<std::uint64_t>::max(),
        0,
        0,
        2,
        false));
}

TEST_CASE("FilesystemCollector filters pseudo filesystems and keeps errors")
{
    auto input = MountFixture("sample.txt");
    REQUIRE(input);
    const FakeFilesystemStats stats;

    const auto collection =
        tsm::FilesystemCollector(stats).Collect(input);

    REQUIRE(collection.filesystems.size() == 2);
    CHECK(collection.filesystems[0].mount.mountPoint == "/");
    CHECK(collection.filesystems[1].mount.mountPoint ==
          "/media/My Disk");
    REQUIRE(collection.errors.size() == 1);
    CHECK(collection.errors[0].context == "/srv/data");
}

TEST_CASE("The running Linux mount table exposes usable storage",
          "[integration][linux]")
{
    std::ifstream input("/proc/self/mountinfo");
    REQUIRE(input);
    const tsm::LinuxFilesystemStats stats;

    const auto collection =
        tsm::FilesystemCollector(stats).Collect(input);

    REQUIRE_FALSE(collection.filesystems.empty());
    for (const auto& filesystem : collection.filesystems)
    {
        CHECK(filesystem.usedBytes <= filesystem.totalBytes);
        CHECK(filesystem.usedPercent >= 0.0);
        CHECK(filesystem.usedPercent <= 100.0);
    }
}
