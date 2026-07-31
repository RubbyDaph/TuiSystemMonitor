#include "app/process_action_controller.hpp"

#include <string>

namespace tsm
{
namespace
{

std::string SignalName(ProcessSignal signal)
{
    return signal == ProcessSignal::Terminate
        ? "SIGTERM"
        : "SIGKILL";
}

std::string ErrorMessage(
    const ProcessSignalRequest& request,
    const Error& error)
{
    const std::string pid = std::to_string(request.identity.pid);
    switch (error.kind)
    {
        case ErrorKind::PermissionDenied:
            return "Permission denied for PID " + pid;
        case ErrorKind::Disappeared:
            return "PID " + pid + " no longer exists";
        case ErrorKind::IdentityMismatch:
            return "PID " + pid +
                " now belongs to another process";
        case ErrorKind::InvalidData:
            return "Cannot signal PID " + pid + ": " + error.message;
        case ErrorKind::Io:
        case ErrorKind::Parse:
        case ErrorKind::SystemCall:
            return "Failed to send " + SignalName(request.signal) +
                " to PID " + pid + ": " + error.message;
    }

    return "Failed to signal PID " + pid;
}

}  // namespace

ProcessActionController::ProcessActionController(
    AppState& state,
    const ProcessControl& processControl)
    : state(state),
      processControl(processControl)
{
}

bool ProcessActionController::Execute(
    const ProcessSignalRequest& request)
{
    const auto result = processControl.SendSignal(
        request.identity, request.signal);
    if (!result)
    {
        state.SetProcessActionStatus(
            {ErrorMessage(request, result.GetError()), false});
        return false;
    }

    state.SetProcessActionStatus(
        {SignalName(request.signal) + " sent to PID " +
             std::to_string(request.identity.pid) + " (" +
             request.name + ")",
         true});
    return true;
}

}  // namespace tsm
