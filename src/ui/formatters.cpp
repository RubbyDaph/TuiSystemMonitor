#include "ui/formatters.hpp"

#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace tsm
{

std::string FormatBytes(std::uint64_t bytes)
{
    static constexpr std::array<const char*, 5> units{
        "B", "KiB", "MiB", "GiB", "TiB"};

    double value = static_cast<double>(bytes);
    std::size_t unit = 0;
    while (value >= 1024.0 && unit + 1 < units.size())
    {
        value /= 1024.0;
        ++unit;
    }

    std::ostringstream output;
    if (unit == 0)
    {
        output << bytes;
    }
    else
    {
        output << std::fixed << std::setprecision(1) << value;
    }
    output << ' ' << units[unit];
    return output.str();
}

std::string FormatPercent(std::optional<double> percent)
{
    if (!percent || !std::isfinite(*percent))
    {
        return "-";
    }

    std::ostringstream output;
    output << std::fixed << std::setprecision(1) << *percent << '%';
    return output.str();
}

}  // namespace tsm
