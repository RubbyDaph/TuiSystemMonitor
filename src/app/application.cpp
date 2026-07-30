#include "app/application.hpp"

#include "ui/root_view.hpp"

#include <cpptui.hpp>

namespace tsm {

int Application::run() {
    cpptui::App app;
    app.register_exit_key('q');
    app.run(build_root_view());
    return 0;
}

}  // namespace tsm
