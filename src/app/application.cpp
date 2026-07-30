#include "app/application.hpp"

#include "ui/root_view.hpp"

#include <cpptui.hpp>

namespace tsm
{

int Application::Run()
{
    cpptui::App app;
    app.register_exit_key('q');
    app.run(BuildRootView());
    return 0;
}

}  // namespace tsm
