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
#include "ui/process_tab.hpp"
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
    auto processTab = std::make_shared<ProcessTab>(state);
    auto root = BuildRootView(state, systemTab, processTab);
    app.register_exit_key('q');
    app.register_key(
        'r',
        [&controller, systemTab, processTab]()
        {
            controller.Refresh();
            systemTab->Update();
            processTab->Update();
        });
    app.register_key(
        'p',
        [&state, processTab]()
        {
            if (state.Tab() == ApplicationTab::Processes)
            {
                state.SetSortKey(ProcessSortKey::Pid);
                processTab->Update();
            }
        });
    app.register_key(
        'c',
        [&state, processTab]()
        {
            if (state.Tab() == ApplicationTab::Processes)
            {
                state.SetSortKey(ProcessSortKey::Cpu);
                processTab->Update();
            }
        });
    app.register_key(
        'm',
        [&state, processTab]()
        {
            if (state.Tab() == ApplicationTab::Processes)
            {
                state.SetSortKey(ProcessSortKey::Memory);
                processTab->Update();
            }
        });
    app.add_timer(
        1000,
        [&controller, systemTab, processTab]()
        {
            controller.Refresh();
            systemTab->Update();
            processTab->Update();
        });
    app.run(root);
    return 0;
}

}  // namespace tsm
