#include "linux/linux_process_signal_sender.hpp"
#include "linux/linux_proc_source.hpp"
#include "parsers/process_stat_parser.hpp"
#include "process/process_control.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cerrno>
#include <csignal>
#include <cstdint>
#include <string>
#include <system_error>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace
{

std::string MakeProcessStat(
    tsm::ProcessId pid,
    std::uint64_t startTime)
{
    return std::to_string(pid) +
        " (worker) R 1 2 3 0 -1 4194304 10 0 1 0 "
        "100 50 0 0 20 0 4 0 " +
        std::to_string(startTime) +
        " 4096 10\n";
}

class FakeProcSource final : public tsm::ProcSource
{
public:
    explicit FakeProcSource(
        tsm::Result<std::string> statResult)
        : statResult(std::move(statResult))
    {
    }

    tsm::Result<std::vector<tsm::ProcessId>>
    ListProcessIds() const override
    {
        return tsm::Result<std::vector<tsm::ProcessId>>::Success(
            {});
    }

    tsm::Result<std::string> ReadProcessStat(
        tsm::ProcessId pid) const override
    {
        ++readCount;
        readPid = pid;
        return statResult;
    }

    tsm::Result<std::string> ReadProcessStatus(
        tsm::ProcessId) const override
    {
        return tsm::Result<std::string>::Failure(
            {tsm::ErrorKind::InvalidData,
             "unused",
             {},
             "unused"});
    }

    mutable int readCount{};
    mutable tsm::ProcessId readPid{};

private:
    tsm::Result<std::string> statResult;
};

class FakeSignalSender final : public tsm::ProcessSignalSender
{
public:
    std::error_code Send(
        tsm::ProcessId pid,
        tsm::ProcessSignal signal) const override
    {
        ++sendCount;
        sentPid = pid;
        sentSignal = signal;
        return error;
    }

    std::error_code error;
    mutable int sendCount{};
    mutable tsm::ProcessId sentPid{};
    mutable tsm::ProcessSignal sentSignal{
        tsm::ProcessSignal::Terminate};
};

class ChildProcess
{
public:
    explicit ChildProcess(pid_t pid)
        : pid(pid)
    {
    }

    ~ChildProcess()
    {
        if (pid > 0)
        {
            ::kill(pid, SIGTERM);
            int status{};
            ::waitpid(pid, &status, 0);
        }
    }

    void MarkReaped()
    {
        pid = -1;
    }

private:
    pid_t pid;
};

}  // namespace

TEST_CASE("ProcessControl sends explicitly selected signals")
{
    const tsm::ProcessIdentity identity{123, 456};
    FakeProcSource procSource(
        tsm::Result<std::string>::Success(
            MakeProcessStat(identity.pid, identity.startTimeTicks)));
    FakeSignalSender signalSender;
    const tsm::ProcessControl control(
        procSource, signalSender, 999);

    const auto terminated = control.SendSignal(
        identity, tsm::ProcessSignal::Terminate);
    REQUIRE(terminated);
    CHECK(terminated.Value().identity == identity);
    CHECK(terminated.Value().signal ==
          tsm::ProcessSignal::Terminate);
    CHECK(signalSender.sentPid == identity.pid);
    CHECK(signalSender.sentSignal ==
          tsm::ProcessSignal::Terminate);

    const auto killed = control.SendSignal(
        identity, tsm::ProcessSignal::Kill);
    REQUIRE(killed);
    CHECK(killed.Value().signal == tsm::ProcessSignal::Kill);
    CHECK(signalSender.sentSignal == tsm::ProcessSignal::Kill);
    CHECK(signalSender.sendCount == 2);
}

TEST_CASE("ProcessControl rejects invalid and self PIDs")
{
    FakeProcSource procSource(
        tsm::Result<std::string>::Success(
            MakeProcessStat(100, 1)));
    FakeSignalSender signalSender;
    const tsm::ProcessControl control(
        procSource, signalSender, 100);

    const auto zero = control.SendSignal(
        {0, 1}, tsm::ProcessSignal::Terminate);
    REQUIRE_FALSE(zero);
    CHECK(zero.GetError().kind == tsm::ErrorKind::InvalidData);

    const auto negative = control.SendSignal(
        {-5, 1}, tsm::ProcessSignal::Terminate);
    REQUIRE_FALSE(negative);
    CHECK(negative.GetError().kind ==
          tsm::ErrorKind::InvalidData);

    const auto self = control.SendSignal(
        {100, 1}, tsm::ProcessSignal::Terminate);
    REQUIRE_FALSE(self);
    CHECK(self.GetError().kind == tsm::ErrorKind::InvalidData);
    CHECK(procSource.readCount == 0);
    CHECK(signalSender.sendCount == 0);
}

TEST_CASE("ProcessControl propagates proc read and parse errors")
{
    FakeSignalSender signalSender;

    FakeProcSource missing(
        tsm::Result<std::string>::Failure(
            {tsm::ErrorKind::Disappeared,
             "/proc/123/stat",
             std::make_error_code(
                 std::errc::no_such_file_or_directory),
             "process disappeared"}));
    const tsm::ProcessControl missingControl(
        missing, signalSender, 999);
    const auto missingResult = missingControl.SendSignal(
        {123, 1}, tsm::ProcessSignal::Terminate);
    REQUIRE_FALSE(missingResult);
    CHECK(missingResult.GetError().kind ==
          tsm::ErrorKind::Disappeared);

    FakeProcSource malformed(
        tsm::Result<std::string>::Success("malformed"));
    const tsm::ProcessControl malformedControl(
        malformed, signalSender, 999);
    const auto malformedResult = malformedControl.SendSignal(
        {123, 1}, tsm::ProcessSignal::Terminate);
    REQUIRE_FALSE(malformedResult);
    CHECK(malformedResult.GetError().kind ==
          tsm::ErrorKind::Parse);
    CHECK(signalSender.sendCount == 0);
}

TEST_CASE("ProcessControl detects reused process identities")
{
    FakeSignalSender signalSender;

    FakeProcSource reusedPid(
        tsm::Result<std::string>::Success(
            MakeProcessStat(124, 456)));
    const tsm::ProcessControl pidControl(
        reusedPid, signalSender, 999);
    const auto pidResult = pidControl.SendSignal(
        {123, 456}, tsm::ProcessSignal::Terminate);
    REQUIRE_FALSE(pidResult);
    CHECK(pidResult.GetError().kind ==
          tsm::ErrorKind::IdentityMismatch);

    FakeProcSource reusedStartTime(
        tsm::Result<std::string>::Success(
            MakeProcessStat(123, 999)));
    const tsm::ProcessControl startTimeControl(
        reusedStartTime, signalSender, 999);
    const auto startTimeResult = startTimeControl.SendSignal(
        {123, 456}, tsm::ProcessSignal::Terminate);
    REQUIRE_FALSE(startTimeResult);
    CHECK(startTimeResult.GetError().kind ==
          tsm::ErrorKind::IdentityMismatch);
    CHECK(signalSender.sendCount == 0);
}

TEST_CASE("ProcessControl maps signal system errors")
{
    const tsm::ProcessIdentity identity{123, 456};

    const auto checkError =
        [&identity](int errorNumber, tsm::ErrorKind expectedKind)
        {
            FakeProcSource procSource(
                tsm::Result<std::string>::Success(
                    MakeProcessStat(
                        identity.pid,
                        identity.startTimeTicks)));
            FakeSignalSender signalSender;
            signalSender.error = {
                errorNumber, std::generic_category()};
            const tsm::ProcessControl control(
                procSource, signalSender, 999);

            const auto result = control.SendSignal(
                identity, tsm::ProcessSignal::Terminate);
            REQUIRE_FALSE(result);
            CHECK(result.GetError().kind == expectedKind);
            CHECK(result.GetError().code.value() == errorNumber);
            CHECK(signalSender.sendCount == 1);
        };

    checkError(EPERM, tsm::ErrorKind::PermissionDenied);
    checkError(EACCES, tsm::ErrorKind::PermissionDenied);
    checkError(ESRCH, tsm::ErrorKind::Disappeared);
    checkError(EINVAL, tsm::ErrorKind::SystemCall);
}

TEST_CASE("ProcessControl terminates only its own child process",
          "[integration][linux]")
{
    const pid_t childPid = ::fork();
    REQUIRE(childPid >= 0);

    if (childPid == 0)
    {
        std::signal(SIGTERM, SIG_DFL);
        for (;;)
        {
            ::pause();
        }
    }

    ChildProcess child(childPid);
    const tsm::LinuxProcSource procSource;
    const auto statText = procSource.ReadProcessStat(
        static_cast<tsm::ProcessId>(childPid));
    REQUIRE(statText);

    const auto stat = tsm::ProcessStatParser{}.Parse(
        statText.Value());
    REQUIRE(stat);

    const tsm::LinuxProcessSignalSender signalSender;
    const tsm::ProcessControl control(
        procSource,
        signalSender,
        static_cast<tsm::ProcessId>(::getpid()));
    const auto result = control.SendSignal(
        stat.Value().identity,
        tsm::ProcessSignal::Terminate);
    REQUIRE(result);

    int status{};
    REQUIRE(::waitpid(childPid, &status, 0) == childPid);
    child.MarkReaped();
    CHECK(WIFSIGNALED(status));
    CHECK(WTERMSIG(status) == SIGTERM);
}
