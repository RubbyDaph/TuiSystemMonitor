#include "ui/root_view.hpp"

#include "app/app_state.hpp"
#include "ui/process_tab.hpp"
#include "ui/system_tab.hpp"

#include <cpptui.hpp>

#include <memory>

namespace tsm
{

std::shared_ptr<cpptui::Widget> BuildRootView(AppState& state)
{
    return BuildRootView(
        state,
        std::make_shared<SystemTab>(state),
        std::make_shared<ProcessTab>(state));
}

std::shared_ptr<cpptui::Widget> BuildRootView(
    AppState& state,
    const std::shared_ptr<SystemTab>& systemContent,
    const std::shared_ptr<ProcessTab>& processContent)
{
    auto systemTab = std::make_shared<cpptui::Border>(
            cpptui::BorderStyle::Rounded);
    systemTab->set_title("System statistics");
    systemTab->add(systemContent);

    auto processTab = std::make_shared<cpptui::Border>(
            cpptui::BorderStyle::Rounded);
    processTab->set_title("Processes");
    processTab->add(processContent);

    auto tabs = std::make_shared<cpptui::Tabs>();
    tabs->add_tab("System", systemTab);
    tabs->add_tab("Processes", processTab);
    tabs->set_tab(
        state.Tab() == ApplicationTab::System ? 0 : 1);
    tabs->on_change =
        [&state](int index)
        {
            state.SetTab(
                index == 0
                    ? ApplicationTab::System
                    : ApplicationTab::Processes);
        };
    return tabs;
}

}  // namespace tsm
