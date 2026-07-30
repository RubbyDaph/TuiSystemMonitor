include("${PROJECT_SOURCE_DIR}/cmake/Dependencies.cmake")

add_executable(
    tsm_tests
    unit/app_info_test.cpp
    unit/domain_models_test.cpp
    unit/root_view_test.cpp
)

target_include_directories(
    tsm_tests
    SYSTEM PRIVATE
        "${PROJECT_SOURCE_DIR}/ThirdParty"
)

target_link_libraries(
    tsm_tests
    PRIVATE
        tsm_app
        Catch2::Catch2WithMain
)

tsm_configure_target(tsm_tests)

include(Catch)
catch_discover_tests(tsm_tests)
