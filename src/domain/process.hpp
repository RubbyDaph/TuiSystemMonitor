#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace tsm
{

using ProcessId = int;

struct ProcessIdentity
{
    ProcessId pid{};
    std::uint64_t startTimeTicks{};
};

inline bool operator==(const ProcessIdentity& left,
        const ProcessIdentity& right) noexcept
{
    return left.pid == right.pid &&
        left.startTimeTicks == right.startTimeTicks;
}

inline bool operator!=(const ProcessIdentity& left,
        const ProcessIdentity& right) noexcept
{
    return !(left == right);
}

enum class ProcessState
{
    Running,
    Sleeping,
    DiskSleep,
    Stopped,
    TracingStop,
    Zombie,
    Dead,
    Parked,
    Idle,
    Unknown,
};

struct ProcessRawSample
{
    ProcessIdentity identity;
    std::string name;
    std::uint32_t effectiveUid{};
    ProcessState state{ProcessState::Unknown};
    std::uint64_t userTicks{};
    std::uint64_t systemTicks{};
    std::uint64_t residentMemoryBytes{};
};

struct ProcessInfo
{
    ProcessIdentity identity;
    std::string name;
    std::string user;
    ProcessState state{ProcessState::Unknown};
    std::optional<double> cpuPercent;
    std::uint64_t residentMemoryBytes{};
};

}  // namespace tsm
