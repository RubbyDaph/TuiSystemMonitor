#include "linux/linux_user_resolver.hpp"

#include <cerrno>
#include <limits>
#include <pwd.h>
#include <string>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace tsm
{

Result<std::string> LinuxUserResolver::Resolve(
    std::uint32_t effectiveUid) const
{
    long suggestedSize = sysconf(_SC_GETPW_R_SIZE_MAX);
    std::size_t bufferSize =
        suggestedSize > 0 ? static_cast<std::size_t>(suggestedSize) : 1024;
    constexpr std::size_t maximumBufferSize = 1024 * 1024;

    while (bufferSize <= maximumBufferSize)
    {
        std::vector<char> buffer(bufferSize);
        struct passwd entry
        {
        };
        struct passwd* result = nullptr;
        const int status = getpwuid_r(
            static_cast<uid_t>(effectiveUid),
            &entry,
            buffer.data(),
            buffer.size(),
            &result);

        if (status == 0)
        {
            return Result<std::string>::Success(
                result == nullptr
                    ? std::to_string(effectiveUid)
                    : std::string(result->pw_name));
        }
        if (status != ERANGE)
        {
            return Result<std::string>::Failure(
                {ErrorKind::SystemCall,
                 std::to_string(effectiveUid),
                 std::error_code(status, std::generic_category()),
                 "getpwuid_r failed"});
        }

        bufferSize *= 2;
    }

    return Result<std::string>::Failure(
        {ErrorKind::InvalidData,
         std::to_string(effectiveUid),
         {},
         "user database entry exceeds the supported size"});
}

}  // namespace tsm
