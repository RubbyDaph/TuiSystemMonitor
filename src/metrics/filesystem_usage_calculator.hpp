#pragma once

#include "domain/filesystem.hpp"
#include "domain/result.hpp"

#include <cstdint>

namespace tsm
{

class FilesystemUsageCalculator
{
public:
    Result<FilesystemUsage> Calculate(
        const MountInfo& mount,
        std::uint64_t blocks,
        std::uint64_t freeBlocks,
        std::uint64_t availableBlocks,
        std::uint64_t fragmentSize,
        bool systemReadOnly) const;
};

}  // namespace tsm
