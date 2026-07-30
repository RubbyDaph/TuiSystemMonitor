#include "metrics/process_sorter.hpp"

#include <algorithm>
#include <cmath>

namespace tsm
{
namespace
{

bool HasKnownCpu(const ProcessInfo& process)
{
    return process.cpuPercent &&
           std::isfinite(*process.cpuPercent);
}

}  // namespace

void ProcessSorter::Sort(
    std::vector<ProcessInfo>& processes,
    ProcessSortKey key) const
{
    std::stable_sort(
        processes.begin(),
        processes.end(),
        [key](const ProcessInfo& left, const ProcessInfo& right)
        {
            if (key == ProcessSortKey::Pid)
            {
                return left.identity.pid < right.identity.pid;
            }

            if (key == ProcessSortKey::Memory &&
                left.residentMemoryBytes != right.residentMemoryBytes)
            {
                return left.residentMemoryBytes >
                       right.residentMemoryBytes;
            }

            if (key == ProcessSortKey::Cpu)
            {
                const bool leftKnown = HasKnownCpu(left);
                const bool rightKnown = HasKnownCpu(right);
                if (leftKnown != rightKnown)
                {
                    return leftKnown;
                }
                if (leftKnown &&
                    *left.cpuPercent != *right.cpuPercent)
                {
                    return *left.cpuPercent > *right.cpuPercent;
                }
            }

            return left.identity.pid < right.identity.pid;
        });
}

}  // namespace tsm
