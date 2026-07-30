#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace tsm {

using ProcessId = int;

struct ProcessIdentity {
    ProcessId pid{};
    std::uint64_t start_time_ticks{};
};

inline bool operator==(const ProcessIdentity& left,
                       const ProcessIdentity& right) noexcept {
    return left.pid == right.pid &&
           left.start_time_ticks == right.start_time_ticks;
}

inline bool operator!=(const ProcessIdentity& left,
                       const ProcessIdentity& right) noexcept {
    return !(left == right);
}

enum class ProcessState {
    running,
    sleeping,
    disk_sleep,
    stopped,
    tracing_stop,
    zombie,
    dead,
    parked,
    idle,
    unknown,
};

struct ProcessRawSample {
    ProcessIdentity identity;
    std::string name;
    std::uint32_t effective_uid{};
    ProcessState state{ProcessState::unknown};
    std::uint64_t user_ticks{};
    std::uint64_t system_ticks{};
    std::uint64_t resident_memory_bytes{};
};

struct ProcessInfo {
    ProcessIdentity identity;
    std::string name;
    std::string user;
    ProcessState state{ProcessState::unknown};
    std::optional<double> cpu_percent;
    std::uint64_t resident_memory_bytes{};
};

}  // namespace tsm
