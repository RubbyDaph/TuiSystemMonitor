#include "ui/root_view.hpp"

#include "app/app_info.hpp"

#include <cpptui.hpp>

#include <memory>
#include <string>

namespace tsm
{

std::shared_ptr<cpptui::Widget> BuildRootView()
{
    auto systemContent = std::make_shared<cpptui::Vertical>();

    cpptui::StyledText title;
    title.bold(std::string(ApplicationName()));
    systemContent->add(std::make_shared<cpptui::Label>(title));
    systemContent->add(std::make_shared<cpptui::Label>(
                std::string(ApplicationSummary())));
    systemContent->add(std::make_shared<cpptui::VerticalSpacer>());
    systemContent->add(
            std::make_shared<cpptui::Label>(std::string(QuitHint())));

    auto systemTab = std::make_shared<cpptui::Border>(
            cpptui::BorderStyle::Rounded);
    systemTab->set_title("System statistics");
    systemTab->add(systemContent);

    auto processContent = std::make_shared<cpptui::Vertical>();
    processContent->add(std::make_shared<cpptui::Label>(
                "Process list will be added in a later stage"));
    processContent->add(std::make_shared<cpptui::VerticalSpacer>());
    processContent->add(
            std::make_shared<cpptui::Label>(std::string(QuitHint())));

    auto processTab = std::make_shared<cpptui::Border>(
            cpptui::BorderStyle::Rounded);
    processTab->set_title("Processes");
    processTab->add(processContent);

    auto tabs = std::make_shared<cpptui::Tabs>();
    tabs->add_tab("System", systemTab);
    tabs->add_tab("Processes", processTab);
    return tabs;
}

}  // namespace tsm
