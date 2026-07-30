#pragma once

#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <variant>

namespace tsm {

enum class ErrorKind {
    io,
    parse,
    permission_denied,
    disappeared,
    invalid_data,
    system_call,
};

struct Error {
    ErrorKind kind{ErrorKind::invalid_data};
    std::string context;
    std::error_code code;
    std::string message;
};

template <typename T>
class Result {
public:
    static Result success(T value) {
        return Result(std::move(value));
    }

    static Result failure(Error error) {
        return Result(std::move(error));
    }

    bool has_value() const noexcept {
        return std::holds_alternative<T>(storage_);
    }

    explicit operator bool() const noexcept {
        return has_value();
    }

    T& value() {
        return std::get<T>(storage_);
    }

    const T& value() const {
        return std::get<T>(storage_);
    }

    Error& error() {
        return std::get<Error>(storage_);
    }

    const Error& error() const {
        return std::get<Error>(storage_);
    }

private:
    explicit Result(T value)
        : storage_(std::move(value)) {}

    explicit Result(Error error)
        : storage_(std::move(error)) {}

    std::variant<T, Error> storage_;
};

}  // namespace tsm
