#include "parsers/mountinfo_parser.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace tsm
{
namespace
{

Result<std::string> DecodePath(const std::string& encoded)
{
    std::string decoded;
    decoded.reserve(encoded.size());

    for (std::size_t index = 0; index < encoded.size(); ++index)
    {
        if (encoded[index] != '\\')
        {
            decoded.push_back(encoded[index]);
            continue;
        }

        if (index + 3 >= encoded.size())
        {
            return Result<std::string>::Failure(
                {ErrorKind::Parse,
                 encoded,
                 {},
                 "incomplete mountinfo escape sequence"});
        }

        int value = 0;
        for (std::size_t offset = 1; offset <= 3; ++offset)
        {
            const char digit = encoded[index + offset];
            if (digit < '0' || digit > '7')
            {
                return Result<std::string>::Failure(
                    {ErrorKind::Parse,
                     encoded,
                     {},
                     "invalid mountinfo escape sequence"});
            }
            value = value * 8 + (digit - '0');
        }

        decoded.push_back(static_cast<char>(value));
        index += 3;
    }

    return Result<std::string>::Success(std::move(decoded));
}

bool HasOption(const std::string& options, const std::string& expected)
{
    std::istringstream input(options);
    std::string option;
    while (std::getline(input, option, ','))
    {
        if (option == expected)
        {
            return true;
        }
    }
    return false;
}

}  // namespace

Result<std::vector<MountInfo>> MountinfoParser::Parse(
    std::istream& input) const
{
    std::vector<MountInfo> mounts;
    std::string textLine;
    std::size_t lineNumber = 0;

    while (std::getline(input, textLine))
    {
        ++lineNumber;
        if (textLine.empty())
        {
            continue;
        }

        std::istringstream line(textLine);
        std::vector<std::string> fields;
        std::string field;
        while (line >> field)
        {
            fields.push_back(field);
        }

        const auto separator =
            std::find(fields.begin(), fields.end(), "-");
        if (fields.size() < 10 || separator == fields.end())
        {
            return Result<std::vector<MountInfo>>::Failure(
                {ErrorKind::Parse,
                 "/proc/self/mountinfo:" + std::to_string(lineNumber),
                 {},
                 "mountinfo line has an invalid layout"});
        }

        const auto separatorIndex =
            static_cast<std::size_t>(separator - fields.begin());
        if (separatorIndex < 6 || separatorIndex + 2 >= fields.size())
        {
            return Result<std::vector<MountInfo>>::Failure(
                {ErrorKind::Parse,
                 "/proc/self/mountinfo:" + std::to_string(lineNumber),
                 {},
                 "mountinfo separator is misplaced"});
        }

        auto mountPoint = DecodePath(fields[4]);
        auto source = DecodePath(fields[separatorIndex + 2]);
        if (!mountPoint)
        {
            return Result<std::vector<MountInfo>>::Failure(
                mountPoint.GetError());
        }
        if (!source)
        {
            return Result<std::vector<MountInfo>>::Failure(
                source.GetError());
        }

        mounts.push_back(
            {source.Value(),
             mountPoint.Value(),
             fields[separatorIndex + 1],
             HasOption(fields[5], "ro")});
    }

    return Result<std::vector<MountInfo>>::Success(std::move(mounts));
}

Result<std::vector<MountInfo>> MountinfoParser::Parse(
    std::string_view text) const
{
    std::istringstream input{std::string(text)};
    return Parse(input);
}

}  // namespace tsm
