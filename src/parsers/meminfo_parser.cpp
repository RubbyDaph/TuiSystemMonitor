#include "parsers/meminfo_parser.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace tsm
{
namespace
{

constexpr std::uint64_t bytesPerKibibyte = 1024;

using Values = std::unordered_map<std::string, std::uint64_t>;

bool IsRelevantKey(const std::string& key)
{
    return key == "MemTotal" || key == "MemAvailable" ||
        key == "MemFree" || key == "Buffers" ||
        key == "Cached" || key == "SReclaimable" ||
        key == "Shmem" || key == "SwapTotal" ||
        key == "SwapFree";
}

Result<std::uint64_t> ToBytes(const std::string& key,
        const std::string& valueText,
        const std::string& unit)
{
    if (unit != "kB")
    {
        return Result<std::uint64_t>::Failure(
                {ErrorKind::Parse,
                key,
                {},
                "memory value must use the kB unit"});
    }

    std::uint64_t kibibytes{};
    const char* begin = valueText.data();
    const char* end = begin + valueText.size();
    const auto parsed = std::from_chars(begin, end, kibibytes);
    if (parsed.ec != std::errc{} || parsed.ptr != end)
    {
        return Result<std::uint64_t>::Failure(
                {ErrorKind::Parse,
                key,
                {},
                "memory value is not an unsigned integer"});
    }

    if (kibibytes >
            std::numeric_limits<std::uint64_t>::max() /
            bytesPerKibibyte)
    {
        return Result<std::uint64_t>::Failure(
                {ErrorKind::InvalidData,
                key,
                {},
                "memory value overflows bytes"});
    }

    return Result<std::uint64_t>::Success(
            kibibytes * bytesPerKibibyte);
}

Result<std::uint64_t> Required(const Values& values,
        const std::string& key)
{
    const auto found = values.find(key);
    if (found == values.end())
    {
        return Result<std::uint64_t>::Failure(
                {ErrorKind::Parse,
                "/proc/meminfo",
                {},
                "required field is missing: " + key});
    }
    return Result<std::uint64_t>::Success(found->second);
}

std::uint64_t OptionalValue(const Values& values,
        const std::string& key)
{
    const auto found = values.find(key);
    return found == values.end() ? 0 : found->second;
}

std::optional<std::uint64_t> CheckedAdd(std::uint64_t left,
        std::uint64_t right)
{
    if (right > std::numeric_limits<std::uint64_t>::max() - left)
    {
        return std::nullopt;
    }
    return left + right;
}

Result<std::uint64_t> FallbackAvailable(const Values& values,
        std::uint64_t total)
{
    const auto free = Required(values, "MemFree");
    const auto buffers = Required(values, "Buffers");
    const auto cached = Required(values, "Cached");
    if (!free)
    {
        return Result<std::uint64_t>::Failure(free.GetError());
    }
    if (!buffers)
    {
        return Result<std::uint64_t>::Failure(buffers.GetError());
    }
    if (!cached)
    {
        return Result<std::uint64_t>::Failure(cached.GetError());
    }

    auto available = CheckedAdd(free.Value(), buffers.Value());
    if (available)
    {
        available = CheckedAdd(*available, cached.Value());
    }
    if (available)
    {
        available =
            CheckedAdd(*available,
                    OptionalValue(values, "SReclaimable"));
    }
    if (!available)
    {
        return Result<std::uint64_t>::Failure(
                {ErrorKind::InvalidData,
                "/proc/meminfo",
                {},
                "fallback MemAvailable calculation overflowed"});
    }

    const auto shmem = OptionalValue(values, "Shmem");
    const auto afterShmem =
        shmem >= *available ? 0 : *available - shmem;
    return Result<std::uint64_t>::Success(
            std::min(afterShmem, total));
}

}  // namespace

Result<MemoryUsage> MeminfoParser::Parse(std::istream& input) const
{
    Values values;
    std::string textLine;

    while (std::getline(input, textLine))
    {
        std::istringstream line(textLine);
        std::string keyWithColon;
        if (!(line >> keyWithColon) || keyWithColon.back() != ':')
        {
            continue;
        }

        const std::string key =
            keyWithColon.substr(0, keyWithColon.size() - 1);
        if (!IsRelevantKey(key))
        {
            continue;
        }

        std::string valueText;
        std::string unit;
        std::string trailing;
        if (!(line >> valueText >> unit) || (line >> trailing))
        {
            return Result<MemoryUsage>::Failure(
                    {ErrorKind::Parse,
                    key,
                    {},
                    "invalid memory field layout"});
        }
        if (values.find(key) != values.end())
        {
            return Result<MemoryUsage>::Failure(
                    {ErrorKind::Parse,
                    key,
                    {},
                    "duplicate memory field"});
        }

        auto bytes = ToBytes(key, valueText, unit);
        if (!bytes)
        {
            return Result<MemoryUsage>::Failure(bytes.GetError());
        }
        values.emplace(key, bytes.Value());
    }

    const auto total = Required(values, "MemTotal");
    const auto swapTotal = Required(values, "SwapTotal");
    const auto swapFree = Required(values, "SwapFree");
    if (!total)
    {
        return Result<MemoryUsage>::Failure(total.GetError());
    }
    if (!swapTotal)
    {
        return Result<MemoryUsage>::Failure(swapTotal.GetError());
    }
    if (!swapFree)
    {
        return Result<MemoryUsage>::Failure(swapFree.GetError());
    }

    Result<std::uint64_t> available =
        values.find("MemAvailable") != values.end()
        ? Result<std::uint64_t>::Success(
                values.at("MemAvailable"))
        : FallbackAvailable(values, total.Value());
    if (!available)
    {
        return Result<MemoryUsage>::Failure(available.GetError());
    }
    if (available.Value() > total.Value())
    {
        return Result<MemoryUsage>::Failure(
                {ErrorKind::InvalidData,
                "MemAvailable",
                {},
                "available RAM exceeds total RAM"});
    }
    if (swapFree.Value() > swapTotal.Value())
    {
        return Result<MemoryUsage>::Failure(
                {ErrorKind::InvalidData,
                "SwapFree",
                {},
                "free swap exceeds total swap"});
    }

    return Result<MemoryUsage>::Success(
            {total.Value(),
            total.Value() - available.Value(),
            available.Value(),
            swapTotal.Value(),
            swapTotal.Value() - swapFree.Value(),
            swapFree.Value()});
}

Result<MemoryUsage> MeminfoParser::Parse(
        std::string_view text) const
{
    std::istringstream input{std::string(text)};
    return Parse(input);
}

}  // namespace tsm
