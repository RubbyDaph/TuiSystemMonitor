#include "app/app_info.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("The application exposes its basic identity") {
    STATIC_REQUIRE(__cplusplus >= 201703L);

    CHECK(tsm::application_name() == "TUI System Monitor");
    CHECK_FALSE(tsm::application_summary().empty());
    CHECK(tsm::quit_hint().find('q') != std::string_view::npos);
}
