#pragma once

#include "domain/process.hpp"
#include "domain/result.hpp"
#include "linux/linux_proc_source.hpp"

#include <system_error>

namespace tsm
{

class ProcessSignalSender
{
public:
    virtual ~ProcessSignalSender() = default;

    virtual std::error_code Send(
        ProcessId pid,
        ProcessSignal signal) const = 0;
};

class ProcessControl
{
public:
    ProcessControl(
        const ProcSource& procSource,
        const ProcessSignalSender& signalSender,
        ProcessId selfPid);

    Result<ProcessSignalResult> SendSignal(
        const ProcessIdentity& identity,
        ProcessSignal signal) const;

private:
    const ProcSource& procSource;
    const ProcessSignalSender& signalSender;
    ProcessId selfPid;
};

}  // namespace tsm
