#include "parsers/proc_stat_parser.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <sstream>
#include <string>
#include <vector>

namespace tsm {
namespace {

bool is_cpu_label(const std::string& label) {
    if (label == "cpu") {
        return true;
    }

    return label.size() > 3 &&
           label.compare(0, 3, "cpu") == 0 &&
           std::all_of(label.begin() + 3, label.end(), [](unsigned char c) {
               return std::isdigit(c) != 0;
           });
}

Result<CpuTimes> parse_times(std::istringstream& line,
                             const std::string& label) {
    std::vector<std::uint64_t> values;
    std::string token;

    while (line >> token) {
        std::uint64_t value{};
        const char* begin = token.data();
        const char* end = begin + token.size();
        const auto parsed = std::from_chars(begin, end, value);

        if (parsed.ec != std::errc{} || parsed.ptr != end) {
            return Result<CpuTimes>::failure(
                {ErrorKind::parse,
                 label,
                 {},
                 "CPU counter is not an unsigned integer"});
        }
        values.push_back(value);
    }

    if (values.size() < 4) {
        return Result<CpuTimes>::failure(
            {ErrorKind::parse,
             label,
             {},
             "CPU line contains fewer than four counters"});
    }

    values.resize(8, 0);
    return Result<CpuTimes>::success(
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

Result<CpuSample> ProcStatParser::parse(std::istream& input) const {
    CpuSample sample;
    bool aggregate_found = false;
    std::string text_line;

    while (std::getline(input, text_line)) {
        std::istringstream line(text_line);
        std::string label;
        if (!(line >> label) || !is_cpu_label(label)) {
            continue;
        }

        auto times = parse_times(line, label);
        if (!times) {
            return Result<CpuSample>::failure(times.error());
        }

        if (label == "cpu") {
            if (aggregate_found) {
                return Result<CpuSample>::failure(
                    {ErrorKind::parse,
                     "/proc/stat",
                     {},
                     "duplicate aggregate CPU line"});
            }
            sample.aggregate = times.value();
            aggregate_found = true;
        } else {
            sample.cores.push_back({label, times.value()});
        }
    }

    if (!aggregate_found) {
        return Result<CpuSample>::failure(
            {ErrorKind::parse,
             "/proc/stat",
             {},
             "aggregate CPU line is missing"});
    }

    return Result<CpuSample>::success(std::move(sample));
}

Result<CpuSample> ProcStatParser::parse(std::string_view text) const {
    std::istringstream input{std::string(text)};
    return parse(input);
}

}  // namespace tsm
