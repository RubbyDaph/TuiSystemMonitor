#pragma once

#include "domain/process.hpp"

#include <vector>

namespace tsm
{

class ProcessSorter
{
public:
    void Sort(
        std::vector<ProcessInfo>& processes,
        ProcessSortKey key) const;
};

}  // namespace tsm
