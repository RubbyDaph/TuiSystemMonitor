#include "process/process_control.hpp"

#include "parsers/process_stat_parser.hpp"

#include <cerrno>
#include <string>

namespace tsm
{
namespace
{

Error SignalError(
    ProcessId pid,
    const std::error_code& code)
{
    ErrorKind kind = ErrorKind::SystemCall;
    if (code.value() == EPERM || code.value() == EACCES)
    {
        kind = ErrorKind::PermissionDenied;
    }
    else if (code.value() == ESRCH)
    {
        kind = ErrorKind::Disappeared;
    }

    return {
        kind,
        "process " + std::to_string(pid),
        code,
        code.message()};
}

}  // namespace

ProcessControl::ProcessControl(
    const ProcSource& procSource,
    const ProcessSignalSender& signalSender,
    ProcessId selfPid)
    : procSource(procSource),
      signalSender(signalSender),
      selfPid(selfPid)
{
}

Result<ProcessSignalResult> ProcessControl::Terminate(
    const ProcessIdentity& identity) const
{
    const std::string context =
        "process " + std::to_string(identity.pid);

    if (identity.pid <= 0)
    {
        return Result<ProcessSignalResult>::Failure(
            {ErrorKind::InvalidData,
             context,
             {},
             "process PID must be positive"});
    }

    if (identity.pid == selfPid)
    {
        return Result<ProcessSignalResult>::Failure(
            {ErrorKind::InvalidData,
             context,
             {},
             "refusing to terminate the monitor process"});
    }

    if (identity.pid == 1)
    {
        return Result<ProcessSignalResult>::Failure(
            {ErrorKind::InvalidData,
             context,
             {},
             "refusing to terminate the system init process"});
    }

    auto statText = procSource.ReadProcessStat(identity.pid);
    if (!statText)
    {
        return Result<ProcessSignalResult>::Failure(
            statText.GetError());
    }

    auto stat = ProcessStatParser{}.Parse(statText.Value());
    if (!stat)
    {
        return Result<ProcessSignalResult>::Failure(
            stat.GetError());
    }

    if (stat.Value().identity != identity)
    {
        return Result<ProcessSignalResult>::Failure(
            {ErrorKind::IdentityMismatch,
             context,
             {},
             "PID belongs to a different process"});
    }

    const std::error_code signalError =
        signalSender.Send(identity.pid);
    if (signalError)
    {
        return Result<ProcessSignalResult>::Failure(
            SignalError(identity.pid, signalError));
    }

    return Result<ProcessSignalResult>::Success(
        {identity});
}

}  // namespace tsm
