#include "app/app_info.hpp"

namespace tsm {

std::string_view application_name() noexcept {
    return "TUI System Monitor";
}

std::string_view application_summary() noexcept {
    return "Linux system statistics and processes";
}

std::string_view quit_hint() noexcept {
    return "Press q to quit";
}

}  // namespace tsm
