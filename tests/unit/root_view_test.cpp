#include "app/app_state.hpp"
#include "ui/root_view.hpp"

#include <cpptui.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("The root view can be laid out and rendered")
{
    tsm::AppState state;
    auto root = tsm::BuildRootView(state);
    REQUIRE(root);

    auto tabs = std::dynamic_pointer_cast<cpptui::Tabs>(root);
    REQUIRE(tabs);
    REQUIRE(tabs->tab_names.size() == 2);
    CHECK(tabs->tab_names[0].plain_text() == "System");
    CHECK(tabs->tab_names[1].plain_text() == "Processes");
    CHECK(tabs->current_tab == 0);

    root->width = 80;
    root->height = 24;

    auto container = std::dynamic_pointer_cast<cpptui::Container>(root);
    REQUIRE(container);
    container->layout();

    cpptui::Buffer buffer(80, 24);
    CHECK_NOTHROW(root->render(buffer));
}

TEST_CASE("The top tabs switch between system and processes")
{
    tsm::AppState state;
    auto tabs =
        std::dynamic_pointer_cast<cpptui::Tabs>(
            tsm::BuildRootView(state));
    REQUIRE(tabs);

    cpptui::Event next;
    next.type = cpptui::EventType::Key;
    next.key = ']';
    CHECK(tabs->on_event(next));
    CHECK(tabs->current_tab == 1);
    CHECK(state.Tab() == tsm::ApplicationTab::Processes);

    cpptui::Event previous;
    previous.type = cpptui::EventType::Key;
    previous.key = '[';
    CHECK(tabs->on_event(previous));
    CHECK(tabs->current_tab == 0);
    CHECK(state.Tab() == tsm::ApplicationTab::System);
}
