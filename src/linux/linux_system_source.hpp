#pragma once

#include "domain/result.hpp"

#include <string>

namespace tsm
{

class SystemSource
{
public:
    virtual ~SystemSource() = default;

    virtual Result<std::string> ReadCpuStat() const = 0;
    virtual Result<std::string> ReadMeminfo() const = 0;
    virtual Result<std::string> ReadMountinfo() const = 0;
};

class LinuxSystemSource final : public SystemSource
{
public:
    Result<std::string> ReadCpuStat() const override;
    Result<std::string> ReadMeminfo() const override;
    Result<std::string> ReadMountinfo() const override;

private:
    Result<std::string> ReadFile(const std::string& path) const;
};

}  // namespace tsm
