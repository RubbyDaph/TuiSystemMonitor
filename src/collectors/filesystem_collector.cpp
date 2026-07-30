#include "collectors/filesystem_collector.hpp"

#include "parsers/mountinfo_parser.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace tsm
{

bool FilesystemFilter::ShouldInclude(const MountInfo& mount) const
{
    static constexpr std::array<std::string_view, 19> excluded{
        "proc",
        "sysfs",
        "devtmpfs",
        "devpts",
        "tmpfs",
        "ramfs",
        "cgroup",
        "cgroup2",
        "mqueue",
        "securityfs",
        "pstore",
        "tracefs",
        "debugfs",
        "configfs",
        "fusectl",
        "hugetlbfs",
        "binfmt_misc",
        "autofs",
        "rpc_pipefs",
    };

    return std::find(
               excluded.begin(),
               excluded.end(),
               mount.filesystemType) == excluded.end() &&
           mount.source.rfind("/dev/", 0) == 0 &&
           mount.source.rfind("/dev/loop", 0) != 0;
}

FilesystemCollector::FilesystemCollector(
    const FilesystemStatsSource& statsSource)
    : statsSource(statsSource)
{
}

FilesystemCollection FilesystemCollector::Collect(
    std::istream& mountinfo) const
{
    FilesystemCollection collection;
    auto parsed = MountinfoParser{}.Parse(mountinfo);
    if (!parsed)
    {
        collection.errors.push_back(parsed.GetError());
        collection.available = false;
        return collection;
    }

    for (const auto& mount : parsed.Value())
    {
        if (!filter.ShouldInclude(mount))
        {
            continue;
        }

        auto usage = statsSource.Read(mount);
        if (!usage)
        {
            collection.errors.push_back(usage.GetError());
            continue;
        }
        collection.filesystems.push_back(std::move(usage.Value()));
    }

    return collection;
}

}  // namespace tsm
