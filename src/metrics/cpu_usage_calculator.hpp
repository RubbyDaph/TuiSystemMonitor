#pragma once

#include "domain/cpu.hpp"
#include "domain/result.hpp"

namespace tsm
{

class CpuUsageCalculator
{
public:
    Result<CpuUsage> Calculate(const CpuSample& previous,
            const CpuSample& current) const;
};

}  // namespace tsm
