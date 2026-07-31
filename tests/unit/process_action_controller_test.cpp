#include "app/process_action_controller.hpp"
#include "process/process_control.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cerrno>
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace
{

std::string MakeActionStat(
    tsm::ProcessId pid,
    std::uint64_t startTime)
{
    return std::to_string(pid) +
        " (worker) R 1 2 3 0 -1 4194304 10 0 1 0 "
        "100 50 0 0 20 0 4 0 " +
        std::to_string(startTime) +
        " 4096 10\n";
}

class ActionProcSource final : public tsm::ProcSource
{
public:
    explicit ActionProcSource(
        tsm::Result<std::string> result)
        : result(std::move(result))
    {
    }

    tsm::Result<std::vector<tsm::ProcessId>>
    ListProcessIds() const override
    {
        return tsm::Result<std::vector<tsm::ProcessId>>::Success(
            {});
    }

    tsm::Result<std::string> ReadProcessStat(
        tsm::ProcessId) const override
    {
        return result;
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

private:
    tsm::Result<std::string> result;
};

class ActionSignalSender final : public tsm::ProcessSignalSender
{
public:
    std::error_code Send(
        tsm::ProcessId) const override
    {
        ++sendCount;
        return error;
    }

    std::error_code error;
    mutable int sendCount{};
};

class GroupProcSource final : public tsm::ProcSource
{
public:
    tsm::Result<std::vector<tsm::ProcessId>>
    ListProcessIds() const override
    {
        return tsm::Result<std::vector<tsm::ProcessId>>::Success(
            {});
    }

    tsm::Result<std::string> ReadProcessStat(
        tsm::ProcessId pid) const override
    {
        return tsm::Result<std::string>::Success(
            MakeActionStat(
                pid,
                static_cast<std::uint64_t>(pid + 100)));
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
};

}  // namespace

TEST_CASE("ProcessActionController stores successful actions")
{
    const tsm::ProcessKillallRequest request{
        "worker",
        {{123, 456}}};
    ActionProcSource procSource(
        tsm::Result<std::string>::Success(
            MakeActionStat(123, 456)));
    ActionSignalSender signalSender;
    const tsm::ProcessControl processControl(
        procSource, signalSender, 999);
    tsm::AppState state;
    tsm::ProcessActionController controller(
        state, processControl);

    CHECK(controller.Execute(request));
    REQUIRE(state.ProcessStatus());
    CHECK(state.ProcessStatus()->success);
    CHECK(state.ProcessStatus()->message ==
          "Killall requested for worker: 1 process(es)");
    CHECK(signalSender.sendCount == 1);
}

TEST_CASE("ProcessActionController formats process errors")
{
    const tsm::ProcessKillallRequest request{
        "worker",
        {{123, 456}}};

    const auto checkProcError =
        [&request](
            tsm::ErrorKind kind,
            const std::string& expectedText)
        {
            ActionProcSource procSource(
                tsm::Result<std::string>::Failure(
                    {kind,
                     "/proc/123/stat",
                     {},
                     "test error"}));
            ActionSignalSender signalSender;
            const tsm::ProcessControl processControl(
                procSource, signalSender, 999);
            tsm::AppState state;
            tsm::ProcessActionController controller(
                state, processControl);

            CHECK_FALSE(controller.Execute(request));
            REQUIRE(state.ProcessStatus());
            CHECK_FALSE(state.ProcessStatus()->success);
            CHECK(state.ProcessStatus()->message.find(expectedText) !=
                  std::string::npos);
            CHECK(signalSender.sendCount == 0);
        };

    checkProcError(tsm::ErrorKind::Disappeared, "no longer running");
    checkProcError(tsm::ErrorKind::Io, "Failed to terminate");

    ActionProcSource malformed(
        tsm::Result<std::string>::Success("malformed"));
    ActionSignalSender malformedSender;
    const tsm::ProcessControl malformedControl(
        malformed, malformedSender, 999);
    tsm::AppState malformedState;
    tsm::ProcessActionController malformedController(
        malformedState, malformedControl);
    CHECK_FALSE(malformedController.Execute(request));
    REQUIRE(malformedState.ProcessStatus());
    CHECK(malformedState.ProcessStatus()->message.find(
              "Failed to terminate") != std::string::npos);

    ActionProcSource reused(
        tsm::Result<std::string>::Success(
            MakeActionStat(123, 999)));
    ActionSignalSender reusedSender;
    const tsm::ProcessControl reusedControl(
        reused, reusedSender, 999);
    tsm::AppState reusedState;
    tsm::ProcessActionController reusedController(
        reusedState, reusedControl);
    CHECK_FALSE(reusedController.Execute(request));
    REQUIRE(reusedState.ProcessStatus());
    CHECK(reusedState.ProcessStatus()->message.find(
              "has changed") != std::string::npos);
}

TEST_CASE("ProcessActionController formats permission and self errors")
{
    const tsm::ProcessKillallRequest request{
        "worker",
        {{123, 456}}};
    ActionProcSource procSource(
        tsm::Result<std::string>::Success(
            MakeActionStat(123, 456)));
    ActionSignalSender deniedSender;
    deniedSender.error = {
        EPERM, std::generic_category()};
    const tsm::ProcessControl deniedControl(
        procSource, deniedSender, 999);
    tsm::AppState deniedState;
    tsm::ProcessActionController deniedController(
        deniedState, deniedControl);

    CHECK_FALSE(deniedController.Execute(request));
    REQUIRE(deniedState.ProcessStatus());
    CHECK(deniedState.ProcessStatus()->message ==
          "Permission denied for worker");

    ActionSignalSender selfSender;
    const tsm::ProcessControl selfControl(
        procSource, selfSender, 123);
    tsm::AppState selfState;
    tsm::ProcessActionController selfController(
        selfState, selfControl);

    CHECK_FALSE(selfController.Execute(request));
    REQUIRE(selfState.ProcessStatus());
    CHECK(selfState.ProcessStatus()->message.find(
              "refusing to terminate the monitor process") !=
          std::string::npos);
    CHECK(selfSender.sendCount == 0);
}

TEST_CASE("ProcessActionController formats signal system errors")
{
    const tsm::ProcessKillallRequest request{
        "worker",
        {{123, 456}}};
    ActionProcSource procSource(
        tsm::Result<std::string>::Success(
            MakeActionStat(123, 456)));
    ActionSignalSender signalSender;
    signalSender.error = {
        EINVAL, std::generic_category()};
    const tsm::ProcessControl processControl(
        procSource, signalSender, 999);
    tsm::AppState state;
    tsm::ProcessActionController controller(
        state, processControl);

    CHECK_FALSE(controller.Execute(request));
    REQUIRE(state.ProcessStatus());
    CHECK_FALSE(state.ProcessStatus()->success);
    CHECK(state.ProcessStatus()->message.find(
              "Failed to terminate") != std::string::npos);
    CHECK(signalSender.sendCount == 1);
}

TEST_CASE("ProcessActionController terminates every unique match")
{
    const tsm::ProcessKillallRequest request{
        "worker",
        {{10, 110}, {20, 120}, {10, 110}}};
    const GroupProcSource procSource;
    ActionSignalSender signalSender;
    const tsm::ProcessControl processControl(
        procSource, signalSender, 999);
    tsm::AppState state;
    tsm::ProcessActionController controller(
        state, processControl);

    CHECK(controller.Execute(request));
    CHECK(signalSender.sendCount == 2);
    REQUIRE(state.ProcessStatus());
    CHECK(state.ProcessStatus()->success);
    CHECK(state.ProcessStatus()->message.find("2 process") !=
          std::string::npos);
}

TEST_CASE("ProcessActionController reports partial killall")
{
    const tsm::ProcessKillallRequest request{
        "worker",
        {{10, 110}, {1, 101}}};
    const GroupProcSource procSource;
    ActionSignalSender signalSender;
    const tsm::ProcessControl processControl(
        procSource, signalSender, 999);
    tsm::AppState state;
    tsm::ProcessActionController controller(
        state, processControl);

    CHECK_FALSE(controller.Execute(request));
    CHECK(signalSender.sendCount == 1);
    REQUIRE(state.ProcessStatus());
    CHECK_FALSE(state.ProcessStatus()->success);
    CHECK(state.ProcessStatus()->message.find(
              "partially completed") != std::string::npos);
}

TEST_CASE("ProcessActionController rejects an empty killall")
{
    const GroupProcSource procSource;
    ActionSignalSender signalSender;
    const tsm::ProcessControl processControl(
        procSource, signalSender, 999);
    tsm::AppState state;
    tsm::ProcessActionController controller(
        state, processControl);

    CHECK_FALSE(controller.Execute({"worker", {}}));
    CHECK(signalSender.sendCount == 0);
    REQUIRE(state.ProcessStatus());
    CHECK(state.ProcessStatus()->message.find("No matching") !=
          std::string::npos);
}
