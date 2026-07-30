#include "app/app_info.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("The application exposes its basic identity")
{
    STATIC_REQUIRE(__cplusplus >= 201703L);

    CHECK(tsm::ApplicationName() == "TUI System Monitor");
    CHECK_FALSE(tsm::ApplicationSummary().empty());
    CHECK(tsm::QuitHint().find('q') != std::string_view::npos);
}
