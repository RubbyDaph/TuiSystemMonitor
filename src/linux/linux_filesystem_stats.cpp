#include "linux/linux_filesystem_stats.hpp"

#include "metrics/filesystem_usage_calculator.hpp"

#include <cerrno>
#include <cstdint>
#include <system_error>
#include <sys/statvfs.h>

namespace tsm
{

Result<FilesystemUsage> LinuxFilesystemStats::Read(
    const MountInfo& mount) const
{
    struct statvfs stats
    {
    };

    if (statvfs(mount.mountPoint.c_str(), &stats) != 0)
    {
        const int errorNumber = errno;
        ErrorKind kind = ErrorKind::SystemCall;
        if (errorNumber == EACCES || errorNumber == EPERM)
        {
            kind = ErrorKind::PermissionDenied;
        }
        else if (errorNumber == ENOENT || errorNumber == ESTALE)
        {
            kind = ErrorKind::Disappeared;
        }

        return Result<FilesystemUsage>::Failure(
            {kind,
             mount.mountPoint,
             std::error_code(errorNumber, std::generic_category()),
             "statvfs failed"});
    }

    const std::uint64_t fragmentSize =
        stats.f_frsize == 0 ? stats.f_bsize : stats.f_frsize;
    return FilesystemUsageCalculator{}.Calculate(
        mount,
        stats.f_blocks,
        stats.f_bfree,
        stats.f_bavail,
        fragmentSize,
        (stats.f_flag & ST_RDONLY) != 0);
}

}  // namespace tsm
