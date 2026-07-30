#pragma once

#include <memory>

namespace cpptui {
class Widget;
}

namespace tsm {

std::shared_ptr<cpptui::Widget> build_root_view();

}  // namespace tsm
