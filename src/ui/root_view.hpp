#pragma once

#include <memory>

namespace cpptui
{
class Widget;
}

namespace tsm
{

std::shared_ptr<cpptui::Widget> BuildRootView();

}  // namespace tsm
