#pragma once

#include "domain/result.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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
    std::string user;
    ProcessState state{ProcessState::Unknown};
    std::uint64_t userTicks{};
    std::uint64_t systemTicks{};
    std::uint64_t residentMemoryBytes{};
};

struct ProcessStatData
{
    ProcessIdentity identity;
    std::string name;
    ProcessState state{ProcessState::Unknown};
    std::uint64_t userTicks{};
    std::uint64_t systemTicks{};
};

struct ProcessStatusData
{
    std::uint32_t effectiveUid{};
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

enum class ProcessSortKey
{
    Name,
    Cpu,
    Memory,
};

struct ProcessSignalResult
{
    ProcessIdentity identity;
};

struct ProcessSignalRequest
{
    ProcessIdentity identity;
    std::string name;
};

struct ProcessCollection
{
    std::vector<ProcessRawSample> processes;
    std::vector<Error> errors;
    bool available{true};
};

}  // namespace tsm
