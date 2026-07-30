#pragma once

#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <variant>

namespace tsm
{

enum class ErrorKind
{
    Io,
    Parse,
    PermissionDenied,
    Disappeared,
    InvalidData,
    SystemCall,
};

struct Error
{
    ErrorKind kind{ErrorKind::InvalidData};
    std::string context;
    std::error_code code;
    std::string message;
};

template <typename T>
class Result
{
public:
    static Result Success(T value)
    {
        return Result(std::move(value));
    }

    static Result Failure(Error error)
    {
        return Result(std::move(error));
    }

    bool HasValue() const noexcept
    {
        return std::holds_alternative<T>(storage);
    }

    explicit operator bool() const noexcept
    {
        return HasValue();
    }

    T& Value()
    {
        return std::get<T>(storage);
    }

    const T& Value() const
    {
        return std::get<T>(storage);
    }

    Error& GetError()
    {
        return std::get<Error>(storage);
    }

    const Error& GetError() const
    {
        return std::get<Error>(storage);
    }

private:
    explicit Result(T value)
        : storage(std::move(value))
    {}

    explicit Result(Error error)
        : storage(std::move(error))
    {}

    std::variant<T, Error> storage;
};

}  // namespace tsm
