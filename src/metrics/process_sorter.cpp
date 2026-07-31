#include "metrics/process_sorter.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace tsm
{
namespace
{

bool HasKnownCpu(const ProcessInfo& process)
{
    return process.cpuPercent &&
           std::isfinite(*process.cpuPercent);
}

std::string LowerName(const std::string& name)
{
    std::string lower = name;
    std::transform(
        lower.begin(),
        lower.end(),
        lower.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return lower;
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
            const std::string leftName = LowerName(left.name);
            const std::string rightName = LowerName(right.name);

            if (key == ProcessSortKey::Name &&
                leftName != rightName)
            {
                return leftName < rightName;
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

            if (leftName != rightName)
            {
                return leftName < rightName;
            }
            if (left.name != right.name)
            {
                return left.name < right.name;
            }
            return left.identity.pid < right.identity.pid;
        });
}

}  // namespace tsm
