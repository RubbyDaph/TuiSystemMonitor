#pragma once

#include "domain/cpu.hpp"
#include "domain/filesystem.hpp"
#include "domain/memory.hpp"
#include "domain/process.hpp"
#include "domain/result.hpp"

#include <chrono>
#include <optional>
#include <vector>

namespace tsm {

template <typename T>
struct MetricSection {
    std::optional<T> data;
    std::vector<Error> errors;
    bool stale{};
};

struct SystemSnapshot {
    MetricSection<CpuUsage> cpu;
    MetricSection<MemoryUsage> memory;
    MetricSection<std::vector<FilesystemUsage>> filesystems;
};

struct ProcessSnapshot {
    std::vector<ProcessInfo> processes;
    std::vector<Error> errors;
};

struct ApplicationSnapshot {
    std::chrono::steady_clock::time_point collected_at;
    SystemSnapshot system;
    ProcessSnapshot processes;
};

}  // namespace tsm
