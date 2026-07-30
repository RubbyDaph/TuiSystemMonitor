#include "app/app_info.hpp"

namespace tsm
{

std::string_view ApplicationName() noexcept
{
    return "TUI System Monitor";
}

std::string_view ApplicationSummary() noexcept
{
    return "Linux system statistics and processes";
}

std::string_view QuitHint() noexcept
{
    return "Press q to quit";
}

}  // namespace tsm
