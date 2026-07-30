#pragma once

#include "domain/filesystem.hpp"
#include "domain/result.hpp"

#include <iosfwd>
#include <string_view>
#include <vector>

namespace tsm
{

class MountinfoParser
{
public:
    Result<std::vector<MountInfo>> Parse(std::istream& input) const;
    Result<std::vector<MountInfo>> Parse(std::string_view text) const;
};

}  // namespace tsm
