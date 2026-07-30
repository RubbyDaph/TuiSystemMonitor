#include "ui/root_view.hpp"

#include "app/app_info.hpp"

#include <cpptui.hpp>

#include <memory>
#include <string>

namespace tsm {

std::shared_ptr<cpptui::Widget> build_root_view() {
    auto system_content = std::make_shared<cpptui::Vertical>();

    cpptui::StyledText title;
    title.bold(std::string(application_name()));
    system_content->add(std::make_shared<cpptui::Label>(title));
    system_content->add(std::make_shared<cpptui::Label>(
        std::string(application_summary())));
    system_content->add(std::make_shared<cpptui::VerticalSpacer>());
    system_content->add(
        std::make_shared<cpptui::Label>(std::string(quit_hint())));

    auto system_tab = std::make_shared<cpptui::Border>(
        cpptui::BorderStyle::Rounded);
    system_tab->set_title("System statistics");
    system_tab->add(system_content);

    auto process_content = std::make_shared<cpptui::Vertical>();
    process_content->add(std::make_shared<cpptui::Label>(
        "Process list will be added in a later stage"));
    process_content->add(std::make_shared<cpptui::VerticalSpacer>());
    process_content->add(
        std::make_shared<cpptui::Label>(std::string(quit_hint())));

    auto process_tab = std::make_shared<cpptui::Border>(
        cpptui::BorderStyle::Rounded);
    process_tab->set_title("Processes");
    process_tab->add(process_content);

    auto tabs = std::make_shared<cpptui::Tabs>();
    tabs->add_tab("System", system_tab);
    tabs->add_tab("Processes", process_tab);
    return tabs;
}

}  // namespace tsm
