#include "app/app_state.hpp"
#include "ui/formatters.hpp"
#include "ui/system_tab.hpp"

#include <cpptui.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <utility>
#include <vector>

TEST_CASE("System formatters produce compact readable values")
{
    CHECK(tsm::FormatBytes(0) == "0 B");
    CHECK(tsm::FormatBytes(1024) == "1.0 KiB");
    CHECK(tsm::FormatBytes(1024 * 1024) == "1.0 MiB");
    CHECK(tsm::FormatPercent(12.345) == "12.3%");
    CHECK(tsm::FormatPercent(std::nullopt) == "-");
}

TEST_CASE("SystemTab maps an application snapshot to widgets")
{
    tsm::ApplicationSnapshot snapshot;
    snapshot.system.cpu.data =
        tsm::CpuUsage{25.0, {10.0, 40.0}};
    snapshot.system.memory.data =
        tsm::MemoryUsage{
            8ULL * 1024 * 1024 * 1024,
            3ULL * 1024 * 1024 * 1024,
            5ULL * 1024 * 1024 * 1024,
            2ULL * 1024 * 1024 * 1024,
            1ULL * 1024 * 1024 * 1024,
            1ULL * 1024 * 1024 * 1024};
    snapshot.system.filesystems.data =
        std::vector<tsm::FilesystemUsage>{
            {{"/dev/sda1", "/", "ext4", false},
             1000,
             400,
             500,
             44.4,
             false},
            {{"/dev/sda1", "/bind", "ext4", false},
             1000,
             400,
             500,
             44.4,
             false}};

    tsm::AppState state;
    state.ApplySnapshot(std::move(snapshot));
    tsm::SystemTab tab(state);
    tab.Update();

    CHECK(tab.CpuText().find("CPU load") != std::string::npos);
    CHECK(tab.MemoryText() == "RAM:  3.0 GiB / 8.0 GiB");
    CHECK(tab.SwapText() == "Swap: 1.0 GiB / 2.0 GiB");
    CHECK(
        tab.MemoryText().find("available") ==
        std::string::npos);
    CHECK(
        tab.SwapText().find("free") ==
        std::string::npos);
    CHECK(tab.CpuBarCount() == 4);
    CHECK(tab.CpuBarValue(0) == Catch::Approx(0.1F));
    CHECK(tab.CpuBarValue(1) == Catch::Approx(0.4F));
    CHECK(tab.CpuBarValue(2) == Catch::Approx(0.0F));
    CHECK(tab.CpuBarValue(3) == Catch::Approx(0.0F));
    CHECK(tab.CpuPercentText(0) == "10.0%");
    CHECK(tab.CpuPercentText(1) == "40.0%");
    CHECK(tab.MemoryBarValue() == Catch::Approx(0.375F));
    CHECK(tab.MemoryPercentText() == "37.5%");
    CHECK(tab.FilesystemRowCount() == 1);

    tab.Update();
    CHECK(tab.CpuBarValue(0) == Catch::Approx(0.1F));
    CHECK(tab.MemoryBarValue() == Catch::Approx(0.375F));
}

TEST_CASE("SystemTab renders at wide and narrow terminal sizes")
{
    const tsm::AppState state;
    tsm::SystemTab tab(state);

    for (const auto size :
         {std::pair<int, int>{120, 30}, std::pair<int, int>{50, 8}})
    {
        tab.x = 0;
        tab.y = 0;
        tab.width = size.first;
        tab.height = size.second;
        tab.layout();
        cpptui::Buffer buffer(size.first, size.second);
        CHECK_NOTHROW(tab.render(buffer));
    }
}
