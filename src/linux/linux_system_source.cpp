#include "linux/linux_system_source.hpp"

#include <cerrno>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

namespace tsm
{
namespace
{

Error MakeError(const std::string& path, int errorNumber)
{
    ErrorKind kind = ErrorKind::Io;
    if (errorNumber == EACCES || errorNumber == EPERM)
    {
        kind = ErrorKind::PermissionDenied;
    }
    else if (errorNumber == ENOENT)
    {
        kind = ErrorKind::Disappeared;
    }

    return {
        kind,
        path,
        std::error_code(errorNumber, std::generic_category()),
        "unable to read Linux system file"};
}

}  // namespace

Result<std::string> LinuxSystemSource::ReadFile(
    const std::string& path) const
{
    errno = 0;
    std::ifstream input(path);
    if (!input)
    {
        return Result<std::string>::Failure(
            MakeError(path, errno == 0 ? EIO : errno));
    }

    std::string content{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    if (!input.good() && !input.eof())
    {
        return Result<std::string>::Failure(MakeError(path, EIO));
    }
    return Result<std::string>::Success(std::move(content));
}

Result<std::string> LinuxSystemSource::ReadCpuStat() const
{
    return ReadFile("/proc/stat");
}

Result<std::string> LinuxSystemSource::ReadMeminfo() const
{
    return ReadFile("/proc/meminfo");
}

Result<std::string> LinuxSystemSource::ReadMountinfo() const
{
    return ReadFile("/proc/self/mountinfo");
}

}  // namespace tsm
