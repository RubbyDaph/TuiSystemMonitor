#pragma once

#include "domain/cpu.hpp"
#include "domain/process.hpp"
#include "domain/result.hpp"

#include <vector>

namespace tsm
{

class ProcessUsageCalculator
{
public:
    Result<std::vector<ProcessInfo>> Calculate(
        const std::vector<ProcessRawSample>& previousProcesses,
        const std::vector<ProcessRawSample>& currentProcesses,
        const CpuSample& previousCpu,
        const CpuSample& currentCpu) const;
};

}  // namespace tsm
