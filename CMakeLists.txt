cmake_minimum_required(VERSION 3.20)

project(TuiSystemMonitor LANGUAGES CXX)


file(GLOB_RECURSE TSM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(
    tsm
    ${TSM_SOURCES}
)

target_include_directories(
    tsm
    PRIVATE
    src
    ThirdParty
)
