#pragma once

#include <cstdint>
#include <string>

namespace tsm
{

struct MountInfo
{
    std::string source;
    std::string mountPoint;
    std::string filesystemType;
};

struct FilesystemUsage
{
    MountInfo mount;
    std::uint64_t totalBytes{};
    std::uint64_t usedBytes{};
    std::uint64_t availableBytes{};
    double usedPercent{};
    bool readOnly{};
};

}  // namespace tsm
