#include "linux/linux_process_signal_sender.hpp"

#include <cerrno>
#include <csignal>
#include <system_error>

namespace tsm
{

std::error_code LinuxProcessSignalSender::Send(
    ProcessId pid) const
{
    if (::kill(pid, SIGTERM) == 0)
    {
        return {};
    }

    return {errno, std::generic_category()};
}

}  // namespace tsm
