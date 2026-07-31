#include "app/process_action_controller.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

namespace tsm
{
namespace
{

std::string ErrorMessage(
    const ProcessKillallRequest& request,
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
    const ProcessKillallRequest& request)
{
    if (request.identities.empty())
    {
        state.SetProcessActionStatus(
            {"No matching processes for " + request.name, false});
        return false;
    }

    std::vector<ProcessIdentity> processed;
    std::vector<Error> errors;
    std::size_t terminated{};
    for (const auto& identity : request.identities)
    {
        if (std::find(
                processed.begin(),
                processed.end(),
                identity) != processed.end())
        {
            continue;
        }
        processed.push_back(identity);

        const auto result = processControl.Terminate(identity);
        if (result)
        {
            ++terminated;
        }
        else
        {
            errors.push_back(result.GetError());
        }
    }

    if (errors.empty())
    {
        state.SetProcessActionStatus(
            {"Killall requested for " + request.name + ": " +
                 std::to_string(terminated) + " process(es)",
             true});
        return true;
    }

    if (terminated == 0)
    {
        state.SetProcessActionStatus(
            {ErrorMessage(request, errors.front()), false});
        return false;
    }

    state.SetProcessActionStatus(
        {"Killall partially completed for " + request.name +
             ": " + std::to_string(terminated) + " requested, " +
             std::to_string(errors.size()) + " failed",
         false});
    return false;
}

}  // namespace tsm
