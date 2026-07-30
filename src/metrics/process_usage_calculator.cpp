#include "metrics/process_usage_calculator.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>

namespace tsm
{
namespace
{

std::optional<std::uint64_t> CpuTotal(const CpuTimes& times)
{
    const std::array<std::uint64_t, 8> values{
        times.user,
        times.nice,
        times.system,
        times.idle,
        times.ioWait,
        times.irq,
        times.softIrq,
        times.steal,
    };

    std::uint64_t total = 0;
    for (const std::uint64_t value : values)
    {
        if (value > std::numeric_limits<std::uint64_t>::max() - total)
        {
            return std::nullopt;
        }
        total += value;
    }
    return total;
}

std::optional<std::uint64_t> ProcessTotal(
    const ProcessRawSample& process)
{
    if (process.systemTicks >
        std::numeric_limits<std::uint64_t>::max() - process.userTicks)
    {
        return std::nullopt;
    }
    return process.userTicks + process.systemTicks;
}

std::optional<double> ProcessPercent(
    const ProcessRawSample& previous,
    const ProcessRawSample& current,
    std::uint64_t systemDelta,
    std::size_t logicalCpuCount)
{
    const auto previousTotal = ProcessTotal(previous);
    const auto currentTotal = ProcessTotal(current);
    if (!previousTotal || !currentTotal ||
        *currentTotal < *previousTotal || systemDelta == 0)
    {
        return std::nullopt;
    }

    const std::uint64_t processDelta = *currentTotal - *previousTotal;
    return 100.0 * static_cast<double>(processDelta) *
           static_cast<double>(logicalCpuCount) /
           static_cast<double>(systemDelta);
}

}  // namespace

Result<std::vector<ProcessInfo>> ProcessUsageCalculator::Calculate(
    const std::vector<ProcessRawSample>& previousProcesses,
    const std::vector<ProcessRawSample>& currentProcesses,
    const CpuSample& previousCpu,
    const CpuSample& currentCpu) const
{
    const auto previousSystemTotal = CpuTotal(previousCpu.aggregate);
    const auto currentSystemTotal = CpuTotal(currentCpu.aggregate);
    if (!previousSystemTotal || !currentSystemTotal ||
        *currentSystemTotal <= *previousSystemTotal)
    {
        return Result<std::vector<ProcessInfo>>::Failure(
            {ErrorKind::InvalidData,
             "aggregate CPU",
             {},
             "system CPU counters did not advance monotonically"});
    }

    const std::uint64_t systemDelta =
        *currentSystemTotal - *previousSystemTotal;
    const std::size_t logicalCpuCount =
        currentCpu.cores.empty() ? 1 : currentCpu.cores.size();

    std::vector<ProcessInfo> processes;
    processes.reserve(currentProcesses.size());

    for (const auto& current : currentProcesses)
    {
        std::optional<double> cpuPercent;
        const auto previous = std::find_if(
            previousProcesses.begin(),
            previousProcesses.end(),
            [&current](const ProcessRawSample& candidate)
            {
                return candidate.identity == current.identity;
            });

        if (previous != previousProcesses.end())
        {
            cpuPercent = ProcessPercent(
                *previous,
                current,
                systemDelta,
                logicalCpuCount);
        }

        processes.push_back(
            {current.identity,
             current.name,
             current.user,
             current.state,
             cpuPercent,
             current.residentMemoryBytes});
    }

    return Result<std::vector<ProcessInfo>>::Success(
        std::move(processes));
}

}  // namespace tsm
