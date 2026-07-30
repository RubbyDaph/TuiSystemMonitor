#include "metrics/filesystem_usage_calculator.hpp"

#include <limits>

namespace tsm
{
namespace
{

Result<std::uint64_t> Multiply(std::uint64_t left, std::uint64_t right)
{
    if (left != 0 &&
        right > std::numeric_limits<std::uint64_t>::max() / left)
    {
        return Result<std::uint64_t>::Failure(
            {ErrorKind::InvalidData,
             "statvfs",
             {},
             "filesystem size overflows bytes"});
    }
    return Result<std::uint64_t>::Success(left * right);
}

}  // namespace

Result<FilesystemUsage> FilesystemUsageCalculator::Calculate(
    const MountInfo& mount,
    std::uint64_t blocks,
    std::uint64_t freeBlocks,
    std::uint64_t availableBlocks,
    std::uint64_t fragmentSize,
    bool systemReadOnly) const
{
    if (fragmentSize == 0 || freeBlocks > blocks ||
        availableBlocks > freeBlocks)
    {
        return Result<FilesystemUsage>::Failure(
            {ErrorKind::InvalidData,
             mount.mountPoint,
             {},
             "statvfs returned inconsistent block counters"});
    }

    auto totalBytes = Multiply(blocks, fragmentSize);
    auto usedBytes = Multiply(blocks - freeBlocks, fragmentSize);
    auto availableBytes = Multiply(availableBlocks, fragmentSize);
    if (!totalBytes)
    {
        return Result<FilesystemUsage>::Failure(totalBytes.GetError());
    }
    if (!usedBytes)
    {
        return Result<FilesystemUsage>::Failure(usedBytes.GetError());
    }
    if (!availableBytes)
    {
        return Result<FilesystemUsage>::Failure(
            availableBytes.GetError());
    }

    if (usedBytes.Value() >
        std::numeric_limits<std::uint64_t>::max() -
            availableBytes.Value())
    {
        return Result<FilesystemUsage>::Failure(
            {ErrorKind::InvalidData,
             mount.mountPoint,
             {},
             "filesystem percentage base overflows"});
    }

    const std::uint64_t percentageBase =
        usedBytes.Value() + availableBytes.Value();
    const double usedPercent =
        percentageBase == 0
            ? 0.0
            : 100.0 * static_cast<double>(usedBytes.Value()) /
                  static_cast<double>(percentageBase);

    return Result<FilesystemUsage>::Success(
        {mount,
         totalBytes.Value(),
         usedBytes.Value(),
         availableBytes.Value(),
         usedPercent,
         mount.readOnly || systemReadOnly});
}

}  // namespace tsm
