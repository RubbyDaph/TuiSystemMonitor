#pragma once

#include "domain/process.hpp"
#include "domain/result.hpp"

#include <string>
#include <vector>

namespace tsm
{

class ProcSource
{
public:
    virtual ~ProcSource() = default;

    virtual Result<std::vector<ProcessId>> ListProcessIds() const = 0;
    virtual Result<std::string> ReadProcessStat(
        ProcessId pid) const = 0;
    virtual Result<std::string> ReadProcessStatus(
        ProcessId pid) const = 0;
};

class LinuxProcSource final : public ProcSource
{
public:
    explicit LinuxProcSource(std::string procRoot = "/proc");

    Result<std::vector<ProcessId>> ListProcessIds() const override;
    Result<std::string> ReadProcessStat(
        ProcessId pid) const override;
    Result<std::string> ReadProcessStatus(
        ProcessId pid) const override;

private:
    Result<std::string> ReadFile(
        ProcessId pid,
        const std::string& filename) const;

    std::string procRoot;
};

}  // namespace tsm
