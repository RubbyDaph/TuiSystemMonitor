#pragma once

#include "domain/memory.hpp"
#include "domain/result.hpp"

#include <iosfwd>
#include <string_view>

namespace tsm
{

class MeminfoParser
{
public:
    Result<MemoryUsage> Parse(std::istream& input) const;
    Result<MemoryUsage> Parse(std::string_view text) const;
};

}  // namespace tsm
