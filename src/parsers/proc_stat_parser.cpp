#include "parsers/proc_stat_parser.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <string>
#include <vector>

namespace tsm
{
namespace
{

bool IsCpuLabel(const std::string& label)
{
    if (label == "cpu")
    {
        return true;
    }

    return label.size() > 3 &&
        label.compare(0, 3, "cpu") == 0 &&
        std::all_of(label.begin() + 3, label.end(), [](unsigned char c)
                {
                return std::isdigit(c) != 0;
                });
}

Result<CpuTimes> ParseTimes(std::istringstream& line,
        const std::string& label)
{
    std::vector<std::uint64_t> values;
    std::string token;

    while (line >> token)
    {
        std::uint64_t value{};
        const char* begin = token.data();
        const char* end = begin + token.size();
        const auto parsed = std::from_chars(begin, end, value);

        if (parsed.ec != std::errc{} || parsed.ptr != end)
        {
            return Result<CpuTimes>::Failure(
                    {ErrorKind::Parse,
                    label,
                    {},
                    "CPU counter is not an unsigned integer"});
        }
        values.push_back(value);
    }

    if (values.size() < 4)
    {
        return Result<CpuTimes>::Failure(
                {ErrorKind::Parse,
                label,
                {},
                "CPU line contains fewer than four counters"});
    }

    values.resize(8, 0);
    return Result<CpuTimes>::Success(
            {values[0],
            values[1],
            values[2],
            values[3],
            values[4],
            values[5],
            values[6],
            values[7]});
}

}  // namespace

Result<CpuSample> ProcStatParser::Parse(std::istream& input) const
{
    CpuSample sample;
    bool aggregateFound = false;
    std::string textLine;

    while (std::getline(input, textLine))
    {
        std::istringstream line(textLine);
        std::string label;
        if (!(line >> label) || !IsCpuLabel(label))
        {
            continue;
        }

        auto times = ParseTimes(line, label);
        if (!times)
        {
            return Result<CpuSample>::Failure(times.GetError());
        }

        if (label == "cpu")
        {
            if (aggregateFound)
            {
                return Result<CpuSample>::Failure(
                        {ErrorKind::Parse,
                        "/proc/stat",
                        {},
                        "duplicate aggregate CPU line"});
            }
            sample.aggregate = times.Value();
            aggregateFound = true;
        } else
        {
            sample.cores.push_back({label, times.Value()});
        }
    }

    if (!aggregateFound)
    {
        return Result<CpuSample>::Failure(
                {ErrorKind::Parse,
                "/proc/stat",
                {},
                "aggregate CPU line is missing"});
    }

    return Result<CpuSample>::Success(std::move(sample));
}

Result<CpuSample> ProcStatParser::Parse(std::string_view text) const
{
    std::istringstream input{std::string(text)};
    return Parse(input);
}

}  // namespace tsm
