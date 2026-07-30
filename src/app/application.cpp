#include "app/application.hpp"

#include "app/app_state.hpp"
#include "app/command_controller.hpp"
#include "collectors/filesystem_collector.hpp"
#include "collectors/process_collector.hpp"
#include "collectors/snapshot_collector.hpp"
#include "linux/linux_filesystem_stats.hpp"
#include "linux/linux_proc_source.hpp"
#include "linux/linux_system_source.hpp"
#include "linux/linux_user_resolver.hpp"
#include "ui/root_view.hpp"
#include "ui/system_tab.hpp"

#include <cpptui.hpp>

namespace tsm
{

int Application::Run()
{
    LinuxSystemSource systemSource;
    LinuxFilesystemStats filesystemStats;
    FilesystemCollector filesystemCollector(filesystemStats);
    LinuxProcSource procSource;
    LinuxUserResolver userResolver;
    ProcessCollector processCollector(procSource, userResolver);
    SnapshotCollector snapshotCollector(
        systemSource, filesystemCollector, processCollector);
    AppState state;
    CommandController controller(state, snapshotCollector);

    controller.Refresh();

    cpptui::App app;
    auto systemTab = std::make_shared<SystemTab>(state);
    auto root = BuildRootView(systemTab);
    app.register_exit_key('q');
    app.add_timer(
        1000,
        [&controller, systemTab]()
        {
            controller.Refresh();
            systemTab->Update();
        });
    app.run(root);
    return 0;
}

}  // namespace tsm
