#pragma once

#include <memory>

namespace cpptui
{
class Widget;
}

namespace tsm
{

class AppState;
class SystemTab;

std::shared_ptr<cpptui::Widget> BuildRootView(const AppState& state);
std::shared_ptr<cpptui::Widget> BuildRootView(
    const std::shared_ptr<SystemTab>& systemTab);

}  // namespace tsm
