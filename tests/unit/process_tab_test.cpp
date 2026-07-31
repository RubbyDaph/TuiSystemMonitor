#include "app/app_state.hpp"
#include "ui/formatters.hpp"
#include "ui/process_tab.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <optional>
#include <string>

namespace
{

tsm::ApplicationSnapshot MakeSnapshot()
{
    tsm::ApplicationSnapshot snapshot;
    snapshot.processes.processes = {
        {{30, 3},
         "worker",
         "alice",
         tsm::ProcessState::Sleeping,
         12.5,
         4 * 1024 * 1024},
        {{10, 1},
         "compiler",
         "bob",
         tsm::ProcessState::Running,
         150.0,
         32 * 1024 * 1024},
        {{20, 2},
         "new process",
         "carol",
         tsm::ProcessState::Zombie,
         std::nullopt,
         1024},
    };
    return snapshot;
}

}  // namespace

TEST_CASE("ProcessTab displays all required process columns")
{
    tsm::AppState state;
    state.ApplySnapshot(MakeSnapshot());
    tsm::ProcessTab tab(state);

    REQUIRE(tab.RowCount() == 3);
    CHECK(tab.CellText(0, 0) == "10");
    CHECK(tab.CellText(0, 1) == "compiler");
    CHECK(tab.CellText(0, 2) == "bob");
    CHECK(tab.CellText(0, 3) == "150.0%");
    CHECK(tab.CellText(0, 4) == "32.0 MiB");
    CHECK(tab.CellText(0, 5) == "Running");
    CHECK(tab.CellText(2, 3) == "-");
}

TEST_CASE("ProcessTab selection follows identity across sorting")
{
    tsm::AppState state;
    state.ApplySnapshot(MakeSnapshot());
    tsm::ProcessTab tab(state);

    tab.SelectRow(1);
    REQUIRE(state.SelectedProcess());
    CHECK(state.SelectedProcess()->pid == 30);

    state.SetSortKey(tsm::ProcessSortKey::Memory);
    tab.Update();

    REQUIRE(state.SelectedProcess());
    CHECK(state.SelectedProcess()->pid == 30);
    CHECK(tab.SelectedRow() == 1);
    CHECK(tab.CellText(1, 0) == "30");
    CHECK(tab.StatusText().find("Sort: RAM") != std::string::npos);
}

TEST_CASE("ProcessTab handles empty and disappearing process lists")
{
    tsm::AppState state;
    state.ApplySnapshot(MakeSnapshot());
    tsm::ProcessTab tab(state);
    tab.SelectRow(1);

    tsm::ApplicationSnapshot empty;
    state.ApplySnapshot(std::move(empty));
    tab.Update();

    CHECK(tab.RowCount() == 0);
    CHECK(tab.SelectedRow() == 0);
    CHECK_FALSE(state.SelectedProcess());
    CHECK_NOTHROW(tab.SelectRow(0));

    cpptui::Event end;
    end.type = cpptui::EventType::Key;
    end.key = 1004;
    CHECK(tab.on_event(end));
    CHECK(tab.SelectedRow() == 0);
}

TEST_CASE("Process state formatter covers Linux process states")
{
    const std::array states{
        tsm::ProcessState::Running,
        tsm::ProcessState::Sleeping,
        tsm::ProcessState::DiskSleep,
        tsm::ProcessState::Stopped,
        tsm::ProcessState::TracingStop,
        tsm::ProcessState::Zombie,
        tsm::ProcessState::Dead,
        tsm::ProcessState::Parked,
        tsm::ProcessState::Idle,
        tsm::ProcessState::Unknown,
    };

    for (const auto state : states)
    {
        CHECK_FALSE(tsm::FormatProcessState(state).empty());
    }
}

TEST_CASE("ProcessTab keeps the latest process action status")
{
    tsm::AppState state;
    state.ApplySnapshot(MakeSnapshot());
    state.SetProcessActionStatus(
        {"SIGTERM sent to PID 10 (compiler)", true});
    tsm::ProcessTab tab(state);

    CHECK(tab.ActionText() ==
          "Success: SIGTERM sent to PID 10 (compiler)");
    CHECK(tab.StatusText().find("t: terminate") !=
          std::string::npos);
    CHECK(tab.StatusText().find("k: kill") !=
          std::string::npos);

    state.ApplySnapshot(MakeSnapshot());
    tab.Update();
    CHECK(tab.ActionText() ==
          "Success: SIGTERM sent to PID 10 (compiler)");

    state.SetProcessActionStatus(
        {"Permission denied for PID 10", false});
    tab.Update();
    CHECK(tab.ActionText() ==
          "Error: Permission denied for PID 10");
}
