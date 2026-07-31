#include "app/process_action_controller.hpp"

#include <string>

namespace tsm
{
namespace
{

std::string ErrorMessage(
    const ProcessSignalRequest& request,
    const Error& error)
{
    switch (error.kind)
    {
        case ErrorKind::PermissionDenied:
            return "Permission denied for " + request.name;
        case ErrorKind::Disappeared:
            return request.name + " is no longer running";
        case ErrorKind::IdentityMismatch:
            return request.name + " has changed";
        case ErrorKind::InvalidData:
            return "Cannot terminate " + request.name +
                ": " + error.message;
        case ErrorKind::Io:
        case ErrorKind::Parse:
        case ErrorKind::SystemCall:
            return "Failed to terminate " + request.name +
                ": " + error.message;
    }

    return "Failed to terminate " + request.name;
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
    const auto result = processControl.Terminate(
        request.identity);
    if (!result)
    {
        state.SetProcessActionStatus(
            {ErrorMessage(request, result.GetError()), false});
        return false;
    }

    state.SetProcessActionStatus(
        {"Termination requested for " + request.name,
         true});
    return true;
}

}  // namespace tsm
