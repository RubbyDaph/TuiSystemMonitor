#pragma once

#include "domain/filesystem.hpp"
#include "linux/linux_filesystem_stats.hpp"

#include <iosfwd>
#include <string_view>

namespace tsm
{

class FilesystemFilter
{
public:
    bool ShouldInclude(std::string_view filesystemType) const;
};

class FilesystemCollector
{
public:
    explicit FilesystemCollector(
        const FilesystemStatsSource& statsSource);

    FilesystemCollection Collect(std::istream& mountinfo) const;

private:
    const FilesystemStatsSource& statsSource;
    FilesystemFilter filter;
};

}  // namespace tsm
