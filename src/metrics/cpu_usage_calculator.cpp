#include "metrics/cpu_usage_calculator.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>

namespace tsm
{
namespace
{

std::optional<std::uint64_t> SumTimes(const CpuTimes& times)
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

    std::uint64_t sum = 0;
    for (const auto value : values)
    {
        if (value > std::numeric_limits<std::uint64_t>::max() - sum)
        {
            return std::nullopt;
        }
        sum += value;
    }
    return sum;
}

std::optional<std::uint64_t> IdleTimes(const CpuTimes& times)
{
    if (times.ioWait >
            std::numeric_limits<std::uint64_t>::max() - times.idle)
    {
        return std::nullopt;
    }
    return times.idle + times.ioWait;
}

std::optional<double> CalculatePercent(const CpuTimes& previous,
        const CpuTimes& current)
{
    const auto previousTotal = SumTimes(previous);
    const auto currentTotal = SumTimes(current);
    const auto previousIdle = IdleTimes(previous);
    const auto currentIdle = IdleTimes(current);

    if (!previousTotal || !currentTotal || !previousIdle ||
            !currentIdle || *currentTotal <= *previousTotal ||
            *currentIdle < *previousIdle)
    {
        return std::nullopt;
    }

    const std::uint64_t totalDelta = *currentTotal - *previousTotal;
    const std::uint64_t idleDelta = *currentIdle - *previousIdle;
    if (idleDelta > totalDelta)
    {
        return std::nullopt;
    }

    const auto busyDelta = totalDelta - idleDelta;
    const double percent =
        100.0 * static_cast<double>(busyDelta) /
        static_cast<double>(totalDelta);
    return std::clamp(percent, 0.0, 100.0);
}

}  // namespace

Result<CpuUsage> CpuUsageCalculator::Calculate(
        const CpuSample& previous,
        const CpuSample& current) const
{
    CpuUsage usage;
    usage.aggregatePercent =
        CalculatePercent(previous.aggregate, current.aggregate);

    if (!usage.aggregatePercent)
    {
        return Result<CpuUsage>::Failure(
                {ErrorKind::InvalidData,
                "aggregate CPU",
                {},
                "CPU counters did not advance monotonically"});
    }

    usage.corePercentages.reserve(current.cores.size());
    for (const auto& currentCore : current.cores)
    {
        const auto previousCore = std::find_if(
                previous.cores.begin(),
                previous.cores.end(),
                [&currentCore](const CpuCoreSample& candidate)
                {
                return candidate.name == currentCore.name;
                });

        if (previousCore == previous.cores.end())
        {
            usage.corePercentages.push_back(std::nullopt);
            continue;
        }

        usage.corePercentages.push_back(
                CalculatePercent(previousCore->times, currentCore.times));
    }

    return Result<CpuUsage>::Success(std::move(usage));
}

}  // namespace tsm
