#pragma once

#include <string_view>

namespace tsm
{

std::string_view ApplicationName() noexcept;
std::string_view ApplicationSummary() noexcept;
std::string_view QuitHint() noexcept;

}  // namespace tsm
