#include "parsers/process_stat_parser.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace tsm
{
namespace
{

template <typename T>
Result<T> ParseUnsigned(
    std::string_view text,
    const std::string& context)
{
    T value{};
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto parsed = std::from_chars(begin, end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end)
    {
        return Result<T>::Failure(
            {ErrorKind::Parse,
             context,
             {},
             "process stat field is not an unsigned integer"});
    }
    return Result<T>::Success(value);
}

ProcessState ParseState(char state)
{
    switch (state)
    {
        case 'R':
            return ProcessState::Running;
        case 'S':
            return ProcessState::Sleeping;
        case 'D':
            return ProcessState::DiskSleep;
        case 'T':
            return ProcessState::Stopped;
        case 't':
            return ProcessState::TracingStop;
        case 'Z':
            return ProcessState::Zombie;
        case 'X':
        case 'x':
            return ProcessState::Dead;
        case 'P':
            return ProcessState::Parked;
        case 'I':
            return ProcessState::Idle;
        default:
            return ProcessState::Unknown;
    }
}

}  // namespace

Result<ProcessStatData> ProcessStatParser::Parse(
    std::string_view text) const
{
    const std::size_t nameBegin = text.find('(');
    const std::size_t nameEnd = text.rfind(") ");
    if (nameBegin == std::string_view::npos ||
        nameEnd == std::string_view::npos ||
        nameEnd <= nameBegin ||
        nameEnd + 3 >= text.size())
    {
        return Result<ProcessStatData>::Failure(
            {ErrorKind::Parse,
             "/proc/[pid]/stat",
             {},
             "process name delimiters are invalid"});
    }

    std::string_view pidText = text.substr(0, nameBegin);
    while (!pidText.empty() && pidText.back() == ' ')
    {
        pidText.remove_suffix(1);
    }
    auto pid = ParseUnsigned<ProcessId>(pidText, "pid");
    if (!pid || pid.Value() <= 0)
    {
        return Result<ProcessStatData>::Failure(
            pid ? Error{ErrorKind::InvalidData,
                        "pid",
                        {},
                        "process PID must be positive"}
                : pid.GetError());
    }

    const char stateCode = text[nameEnd + 2];
    if (text[nameEnd + 3] != ' ')
    {
        return Result<ProcessStatData>::Failure(
            {ErrorKind::Parse,
             "/proc/[pid]/stat",
             {},
             "process state delimiter is invalid"});
    }

    std::istringstream fieldsInput(
        std::string(text.substr(nameEnd + 4)));
    std::vector<std::string> fields;
    std::string field;
    while (fieldsInput >> field)
    {
        fields.push_back(field);
    }
    if (fields.size() < 19)
    {
        return Result<ProcessStatData>::Failure(
            {ErrorKind::Parse,
             "/proc/[pid]/stat",
             {},
             "process stat contains fewer than 22 fields"});
    }

    auto userTicks =
        ParseUnsigned<std::uint64_t>(fields[10], "utime");
    auto systemTicks =
        ParseUnsigned<std::uint64_t>(fields[11], "stime");
    auto startTime =
        ParseUnsigned<std::uint64_t>(fields[18], "starttime");
    if (!userTicks)
    {
        return Result<ProcessStatData>::Failure(
            userTicks.GetError());
    }
    if (!systemTicks)
    {
        return Result<ProcessStatData>::Failure(
            systemTicks.GetError());
    }
    if (!startTime)
    {
        return Result<ProcessStatData>::Failure(
            startTime.GetError());
    }

    return Result<ProcessStatData>::Success(
        {{pid.Value(), startTime.Value()},
         std::string(text.substr(nameBegin + 1, nameEnd - nameBegin - 1)),
         ParseState(stateCode),
         userTicks.Value(),
         systemTicks.Value()});
}

}  // namespace tsm
