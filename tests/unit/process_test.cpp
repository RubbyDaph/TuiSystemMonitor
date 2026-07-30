#include "collectors/process_collector.hpp"
#include "linux/linux_proc_source.hpp"
#include "linux/linux_user_resolver.hpp"
#include "parsers/process_stat_parser.hpp"
#include "parsers/process_status_parser.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace
{

std::string ProcessFixture(const std::string& name)
{
    std::ifstream input(
        std::string(TsmTestFixtureDir) + "/process/" + name);
    REQUIRE(input);
    return {
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
}

class FakeProcSource final : public tsm::ProcSource
{
public:
    tsm::Result<std::vector<tsm::ProcessId>>
    ListProcessIds() const override
    {
        return tsm::Result<std::vector<tsm::ProcessId>>::Success(
            {123, 999});
    }

    tsm::Result<std::string> ReadProcessStat(
        tsm::ProcessId pid) const override
    {
        if (pid == 999)
        {
            return tsm::Result<std::string>::Failure(
                {tsm::ErrorKind::Disappeared,
                 "999/stat",
                 {},
                 "process disappeared"});
        }
        return tsm::Result<std::string>::Success(
            ProcessFixture("stat.txt"));
    }

    tsm::Result<std::string> ReadProcessStatus(
        tsm::ProcessId) const override
    {
        return tsm::Result<std::string>::Success(
            ProcessFixture("status.txt"));
    }
};

class FakeUserResolver final : public tsm::UserResolver
{
public:
    tsm::Result<std::string> Resolve(
        std::uint32_t effectiveUid) const override
    {
        CHECK(effectiveUid == 1001);
        return tsm::Result<std::string>::Success("alice");
    }
};

}  // namespace

TEST_CASE("ProcessStatParser handles spaces and parentheses in names")
{
    const auto normal =
        tsm::ProcessStatParser{}.Parse(ProcessFixture("stat.txt"));
    REQUIRE(normal);
    CHECK(normal.Value().identity.pid == 123);
    CHECK(normal.Value().identity.startTimeTicks == 123456);
    CHECK(normal.Value().name == "worker thread");
    CHECK(normal.Value().state == tsm::ProcessState::Sleeping);
    CHECK(normal.Value().userTicks == 100);
    CHECK(normal.Value().systemTicks == 50);

    const auto parenthesis = tsm::ProcessStatParser{}.Parse(
        ProcessFixture("stat_parenthesis.txt"));
    REQUIRE(parenthesis);
    CHECK(parenthesis.Value().name == "worker ) helper");
    CHECK(parenthesis.Value().state == tsm::ProcessState::Running);
}

TEST_CASE("ProcessStatParser rejects malformed and incomplete data")
{
    CHECK_FALSE(tsm::ProcessStatParser{}.Parse(
        ProcessFixture("malformed_stat.txt")));
    CHECK_FALSE(tsm::ProcessStatParser{}.Parse(
        "0 (invalid) S 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19"));
}

TEST_CASE("ProcessStatusParser reads effective UID and resident memory")
{
    const auto status = tsm::ProcessStatusParser{}.Parse(
        ProcessFixture("status.txt"));
    REQUIRE(status);
    CHECK(status.Value().effectiveUid == 1001);
    CHECK(status.Value().residentMemoryBytes == 2048 * 1024);

    const auto withoutMemory = tsm::ProcessStatusParser{}.Parse(
        ProcessFixture("status_no_rss.txt"));
    REQUIRE(withoutMemory);
    CHECK(withoutMemory.Value().residentMemoryBytes == 0);
}

TEST_CASE("ProcessStatusParser rejects malformed status data")
{
    CHECK_FALSE(tsm::ProcessStatusParser{}.Parse(
        ProcessFixture("malformed_status.txt")));
    CHECK_FALSE(tsm::ProcessStatusParser{}.Parse(
        "Name:\tmissing uid\nVmRSS:\t10 kB\n"));
}

TEST_CASE("ProcessCollector skips a process that disappears")
{
    const FakeProcSource procSource;
    const FakeUserResolver userResolver;
    const auto collection =
        tsm::ProcessCollector(procSource, userResolver).Collect();

    REQUIRE(collection.processes.size() == 1);
    CHECK(collection.processes[0].identity.pid == 123);
    CHECK(collection.processes[0].effectiveUid == 1001);
    CHECK(collection.processes[0].user == "alice");
    CHECK(collection.processes[0].residentMemoryBytes == 2048 * 1024);
    REQUIRE(collection.errors.size() == 1);
    CHECK(collection.errors[0].kind == tsm::ErrorKind::Disappeared);
}

TEST_CASE("Linux user resolver returns a name or numeric UID")
{
    const auto user = tsm::LinuxUserResolver{}.Resolve(
        static_cast<std::uint32_t>(geteuid()));
    REQUIRE(user);
    CHECK_FALSE(user.Value().empty());

    const auto unknown =
        tsm::LinuxUserResolver{}.Resolve(4294967294U);
    REQUIRE(unknown);
    CHECK_FALSE(unknown.Value().empty());
}

TEST_CASE("The running Linux procfs contains the current process",
          "[integration][linux]")
{
    const tsm::LinuxProcSource source;
    const auto processIds = source.ListProcessIds();
    REQUIRE(processIds);

    const auto currentPid = static_cast<tsm::ProcessId>(getpid());
    CHECK(std::find(
              processIds.Value().begin(),
              processIds.Value().end(),
              currentPid) != processIds.Value().end());

    const auto statText = source.ReadProcessStat(currentPid);
    const auto statusText = source.ReadProcessStatus(currentPid);
    REQUIRE(statText);
    REQUIRE(statusText);
    CHECK(tsm::ProcessStatParser{}.Parse(statText.Value()));
    CHECK(tsm::ProcessStatusParser{}.Parse(statusText.Value()));
}
