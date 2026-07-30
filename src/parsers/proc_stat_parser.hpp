#pragma once

#include "domain/cpu.hpp"
#include "domain/result.hpp"

#include <iosfwd>
#include <string_view>

namespace tsm
{

class ProcStatParser
{
public:
    Result<CpuSample> Parse(std::istream& input) const;
    Result<CpuSample> Parse(std::string_view text) const;
};

}  // namespace tsm
