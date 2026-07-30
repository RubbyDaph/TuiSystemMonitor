#pragma once

#include "domain/filesystem.hpp"
#include "domain/result.hpp"

namespace tsm
{

class FilesystemStatsSource
{
public:
    virtual ~FilesystemStatsSource() = default;
    virtual Result<FilesystemUsage> Read(const MountInfo& mount) const = 0;
};

class LinuxFilesystemStats final : public FilesystemStatsSource
{
public:
    Result<FilesystemUsage> Read(const MountInfo& mount) const override;
};

}  // namespace tsm
