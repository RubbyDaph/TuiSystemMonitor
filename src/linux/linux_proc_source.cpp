#include "linux/linux_proc_source.hpp"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

namespace tsm
{
namespace
{

Error MakeError(
    const std::string& context,
    const std::error_code& code)
{
    ErrorKind kind = ErrorKind::Io;
    if (code.value() == EACCES || code.value() == EPERM)
    {
        kind = ErrorKind::PermissionDenied;
    }
    else if (code.value() == ENOENT || code.value() == ESRCH)
    {
        kind = ErrorKind::Disappeared;
    }

    return {kind, context, code, code.message()};
}

}  // namespace

LinuxProcSource::LinuxProcSource(std::string procRoot)
    : procRoot(std::move(procRoot))
{
}

Result<std::vector<ProcessId>> LinuxProcSource::ListProcessIds() const
{
    std::vector<ProcessId> processIds;
    std::error_code error;
    std::filesystem::directory_iterator entries(procRoot, error);
    if (error)
    {
        return Result<std::vector<ProcessId>>::Failure(
            MakeError(procRoot, error));
    }

    for (const auto& entry : entries)
    {
        const std::string name = entry.path().filename().string();
        ProcessId pid{};
        const char* begin = name.data();
        const char* end = begin + name.size();
        const auto parsed = std::from_chars(begin, end, pid);
        if (parsed.ec == std::errc{} && parsed.ptr == end && pid > 0)
        {
            processIds.push_back(pid);
        }
    }

    std::sort(processIds.begin(), processIds.end());
    return Result<std::vector<ProcessId>>::Success(
        std::move(processIds));
}

Result<std::string> LinuxProcSource::ReadFile(
    ProcessId pid,
    const std::string& filename) const
{
    const std::filesystem::path path =
        std::filesystem::path(procRoot) / std::to_string(pid) / filename;
    errno = 0;
    std::ifstream input(path);
    if (!input)
    {
        const int errorNumber = errno == 0 ? EIO : errno;
        return Result<std::string>::Failure(
            MakeError(
                path.string(),
                std::error_code(errorNumber, std::generic_category())));
    }

    std::string content{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    if (!input.good() && !input.eof())
    {
        return Result<std::string>::Failure(
            MakeError(
                path.string(),
                std::make_error_code(std::errc::io_error)));
    }
    return Result<std::string>::Success(std::move(content));
}

Result<std::string> LinuxProcSource::ReadProcessStat(
    ProcessId pid) const
{
    return ReadFile(pid, "stat");
}

Result<std::string> LinuxProcSource::ReadProcessStatus(
    ProcessId pid) const
{
    return ReadFile(pid, "status");
}

}  // namespace tsm
