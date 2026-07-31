#pragma once

#include <memory>

namespace cpptui
{
class Widget;
}

namespace tsm
{

class AppState;
class ProcessTab;
class SystemTab;

std::shared_ptr<cpptui::Widget> BuildRootView(AppState& state);
std::shared_ptr<cpptui::Widget> BuildRootView(
    AppState& state,
    const std::shared_ptr<SystemTab>& systemTab,
    const std::shared_ptr<ProcessTab>& processTab);

}  // namespace tsm
