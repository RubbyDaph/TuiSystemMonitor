#pragma once

#include <cstdint>
#include <string>

namespace tsm {

struct MountInfo {
    std::string source;
    std::string mount_point;
    std::string filesystem_type;
};

struct FilesystemUsage {
    MountInfo mount;
    std::uint64_t total_bytes{};
    std::uint64_t used_bytes{};
    std::uint64_t available_bytes{};
    double used_percent{};
    bool read_only{};
};

}  // namespace tsm
