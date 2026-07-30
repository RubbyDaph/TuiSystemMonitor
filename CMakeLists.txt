cmake_minimum_required(VERSION 3.20)

project(
    TuiSystemMonitor
    VERSION 0.1.0
    DESCRIPTION "A Linux system monitor with a terminal user interface"
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

function(tsm_configure_target target_name)
    set_target_properties(
        ${target_name}
        PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED YES
            CXX_EXTENSIONS NO
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(
            ${target_name}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
        )
    elseif(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    endif()
endfunction()

find_package(Threads REQUIRED)

add_library(tsm_domain INTERFACE)
target_include_directories(
    tsm_domain
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_compile_features(tsm_domain INTERFACE cxx_std_17)

add_library(
    tsm_app
    STATIC
        src/app/app_info.cpp
        src/app/application.cpp
        src/ui/root_view.cpp
)

target_include_directories(
    tsm_app
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_include_directories(
    tsm_app
    SYSTEM PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty"
)

target_link_libraries(tsm_app PUBLIC Threads::Threads tsm_domain)
tsm_configure_target(tsm_app)

add_executable(tsm src/main.cpp)
target_link_libraries(tsm PRIVATE tsm_app)
tsm_configure_target(tsm)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
