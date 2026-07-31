#pragma once

#include "domain/process.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace tsm
{

std::string FormatBytes(std::uint64_t bytes);
std::string FormatPercent(std::optional<double> percent);
std::string FormatProcessState(ProcessState state);

}  // namespace tsm
