#pragma once

#include "domain/process.hpp"
#include "domain/result.hpp"

#include <iosfwd>
#include <string_view>

namespace tsm
{

class ProcessStatusParser
{
public:
    Result<ProcessStatusData> Parse(std::istream& input) const;
    Result<ProcessStatusData> Parse(std::string_view text) const;
};

}  // namespace tsm
