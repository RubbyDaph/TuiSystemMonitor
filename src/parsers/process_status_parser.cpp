#include "parsers/process_status_parser.hpp"

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

namespace tsm
{
namespace
{

template <typename T>
Result<T> ParseUnsigned(
    const std::string& text,
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
             "process status field is not an unsigned integer"});
    }
    return Result<T>::Success(value);
}

Result<std::uint64_t> ToBytes(const std::string& valueText)
{
    auto kibibytes =
        ParseUnsigned<std::uint64_t>(valueText, "VmRSS");
    if (!kibibytes)
    {
        return Result<std::uint64_t>::Failure(
            kibibytes.GetError());
    }
    if (kibibytes.Value() >
        std::numeric_limits<std::uint64_t>::max() / 1024)
    {
        return Result<std::uint64_t>::Failure(
            {ErrorKind::InvalidData,
             "VmRSS",
             {},
             "process resident memory overflows bytes"});
    }
    return Result<std::uint64_t>::Success(
        kibibytes.Value() * 1024);
}

}  // namespace

Result<ProcessStatusData> ProcessStatusParser::Parse(
    std::istream& input) const
{
    std::optional<std::uint32_t> effectiveUid;
    std::uint64_t residentMemoryBytes = 0;
    bool residentMemoryFound = false;
    std::string textLine;

    while (std::getline(input, textLine))
    {
        if (textLine.compare(0, 4, "Uid:") == 0)
        {
            if (effectiveUid)
            {
                return Result<ProcessStatusData>::Failure(
                    {ErrorKind::Parse,
                     "Uid",
                     {},
                     "duplicate process UID field"});
            }

            std::istringstream line(textLine.substr(4));
            std::string realUid;
            std::string parsedEffectiveUid;
            std::string savedUid;
            std::string filesystemUid;
            std::string trailing;
            if (!(line >> realUid >> parsedEffectiveUid >> savedUid >>
                  filesystemUid) ||
                (line >> trailing))
            {
                return Result<ProcessStatusData>::Failure(
                    {ErrorKind::Parse,
                     "Uid",
                     {},
                     "process UID field must contain four values"});
            }

            auto parsed = ParseUnsigned<std::uint32_t>(
                parsedEffectiveUid, "effective UID");
            if (!parsed)
            {
                return Result<ProcessStatusData>::Failure(
                    parsed.GetError());
            }
            effectiveUid = parsed.Value();
        }
        else if (textLine.compare(0, 6, "VmRSS:") == 0)
        {
            if (residentMemoryFound)
            {
                return Result<ProcessStatusData>::Failure(
                    {ErrorKind::Parse,
                     "VmRSS",
                     {},
                     "duplicate process resident memory field"});
            }

            std::istringstream line(textLine.substr(6));
            std::string valueText;
            std::string unit;
            std::string trailing;
            if (!(line >> valueText >> unit) || unit != "kB" ||
                (line >> trailing))
            {
                return Result<ProcessStatusData>::Failure(
                    {ErrorKind::Parse,
                     "VmRSS",
                     {},
                     "process resident memory has an invalid layout"});
            }

            auto bytes = ToBytes(valueText);
            if (!bytes)
            {
                return Result<ProcessStatusData>::Failure(
                    bytes.GetError());
            }
            residentMemoryBytes = bytes.Value();
            residentMemoryFound = true;
        }
    }

    if (!effectiveUid)
    {
        return Result<ProcessStatusData>::Failure(
            {ErrorKind::Parse,
             "/proc/[pid]/status",
             {},
             "effective process UID is missing"});
    }

    return Result<ProcessStatusData>::Success(
        {*effectiveUid, residentMemoryBytes});
}

Result<ProcessStatusData> ProcessStatusParser::Parse(
    std::string_view text) const
{
    std::istringstream input{std::string(text)};
    return Parse(input);
}

}  // namespace tsm
