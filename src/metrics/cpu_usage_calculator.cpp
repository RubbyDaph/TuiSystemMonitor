#include "metrics/cpu_usage_calculator.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>

namespace tsm {
namespace {

std::optional<std::uint64_t> sum_times(const CpuTimes& times) {
    const std::array<std::uint64_t, 8> values{
        times.user,
        times.nice,
        times.system,
        times.idle,
        times.io_wait,
        times.irq,
        times.soft_irq,
        times.steal,
    };

    std::uint64_t sum = 0;
    for (const auto value : values) {
        if (value > std::numeric_limits<std::uint64_t>::max() - sum) {
            return std::nullopt;
        }
        sum += value;
    }
    return sum;
}

std::optional<std::uint64_t> idle_times(const CpuTimes& times) {
    if (times.io_wait >
        std::numeric_limits<std::uint64_t>::max() - times.idle) {
        return std::nullopt;
    }
    return times.idle + times.io_wait;
}

std::optional<double> calculate_percent(const CpuTimes& previous,
                                        const CpuTimes& current) {
    const auto previous_total = sum_times(previous);
    const auto current_total = sum_times(current);
    const auto previous_idle = idle_times(previous);
    const auto current_idle = idle_times(current);

    if (!previous_total || !current_total || !previous_idle ||
        !current_idle || *current_total <= *previous_total ||
        *current_idle < *previous_idle) {
        return std::nullopt;
    }

    const std::uint64_t total_delta = *current_total - *previous_total;
    const std::uint64_t idle_delta = *current_idle - *previous_idle;
    if (idle_delta > total_delta) {
        return std::nullopt;
    }

    const auto busy_delta = total_delta - idle_delta;
    const double percent =
        100.0 * static_cast<double>(busy_delta) /
        static_cast<double>(total_delta);
    return std::clamp(percent, 0.0, 100.0);
}

}  // namespace

Result<CpuUsage> CpuUsageCalculator::calculate(
    const CpuSample& previous,
    const CpuSample& current) const {
    CpuUsage usage;
    usage.aggregate_percent =
        calculate_percent(previous.aggregate, current.aggregate);

    if (!usage.aggregate_percent) {
        return Result<CpuUsage>::failure(
            {ErrorKind::invalid_data,
             "aggregate CPU",
             {},
             "CPU counters did not advance monotonically"});
    }

    usage.core_percentages.reserve(current.cores.size());
    for (const auto& current_core : current.cores) {
        const auto previous_core = std::find_if(
            previous.cores.begin(),
            previous.cores.end(),
            [&current_core](const CpuCoreSample& candidate) {
                return candidate.name == current_core.name;
            });

        if (previous_core == previous.cores.end()) {
            usage.core_percentages.push_back(std::nullopt);
            continue;
        }

        usage.core_percentages.push_back(
            calculate_percent(previous_core->times, current_core.times));
    }

    return Result<CpuUsage>::success(std::move(usage));
}

}  // namespace tsm
