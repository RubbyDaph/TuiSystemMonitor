#pragma once

#include <cstdint>

namespace tsm {

struct MemoryUsage {
    std::uint64_t ram_total_bytes{};
    std::uint64_t ram_used_bytes{};
    std::uint64_t ram_available_bytes{};
    std::uint64_t swap_total_bytes{};
    std::uint64_t swap_used_bytes{};
    std::uint64_t swap_free_bytes{};
};

}  // namespace tsm
