#pragma once

#include <string_view>

namespace tsm {

std::string_view application_name() noexcept;
std::string_view application_summary() noexcept;
std::string_view quit_hint() noexcept;

}  // namespace tsm
