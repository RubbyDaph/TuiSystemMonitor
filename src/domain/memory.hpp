#pragma once

#include <cstdint>

namespace tsm
{

struct MemoryUsage
{
    std::uint64_t ramTotalBytes{};
    std::uint64_t ramUsedBytes{};
    std::uint64_t ramAvailableBytes{};
    std::uint64_t swapTotalBytes{};
    std::uint64_t swapUsedBytes{};
    std::uint64_t swapFreeBytes{};
};

}  // namespace tsm
