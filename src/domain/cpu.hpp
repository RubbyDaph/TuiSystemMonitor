#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tsm {

struct CpuTimes {
    std::uint64_t user{};
    std::uint64_t nice{};
    std::uint64_t system{};
    std::uint64_t idle{};
    std::uint64_t io_wait{};
    std::uint64_t irq{};
    std::uint64_t soft_irq{};
    std::uint64_t steal{};
};

struct CpuCoreSample {
    std::string name;
    CpuTimes times;
};

struct CpuSample {
    CpuTimes aggregate;
    std::vector<CpuCoreSample> cores;
};

struct CpuUsage {
    std::optional<double> aggregate_percent;
    std::vector<std::optional<double>> core_percentages;
};

}  // namespace tsm
