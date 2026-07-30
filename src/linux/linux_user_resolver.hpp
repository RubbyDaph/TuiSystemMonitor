#pragma once

#include "domain/result.hpp"

#include <cstdint>
#include <string>

namespace tsm
{

class UserResolver
{
public:
    virtual ~UserResolver() = default;
    virtual Result<std::string> Resolve(
        std::uint32_t effectiveUid) const = 0;
};

class LinuxUserResolver final : public UserResolver
{
public:
    Result<std::string> Resolve(
        std::uint32_t effectiveUid) const override;
};

}  // namespace tsm
