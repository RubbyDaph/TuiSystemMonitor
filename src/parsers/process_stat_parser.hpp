#pragma once

#include "domain/process.hpp"
#include "domain/result.hpp"

#include <string_view>

namespace tsm
{

class ProcessStatParser
{
public:
    Result<ProcessStatData> Parse(std::string_view text) const;
};

}  // namespace tsm
