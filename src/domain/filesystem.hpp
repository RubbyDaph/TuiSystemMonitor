#pragma once

#include "domain/result.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace tsm
{

struct MountInfo
{
    std::string source;
    std::string mountPoint;
    std::string filesystemType;
    bool readOnly{};
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

struct FilesystemCollection
{
    std::vector<FilesystemUsage> filesystems;
    std::vector<Error> errors;
};

}  // namespace tsm
