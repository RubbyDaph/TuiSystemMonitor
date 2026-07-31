#include "app/application.hpp"

#include "app/app_state.hpp"
#include "app/command_controller.hpp"
#include "app/process_action_controller.hpp"
#include "collectors/filesystem_collector.hpp"
#include "collectors/process_collector.hpp"
#include "collectors/snapshot_collector.hpp"
#include "linux/linux_filesystem_stats.hpp"
#include "linux/linux_process_signal_sender.hpp"
#include "linux/linux_proc_source.hpp"
#include "linux/linux_system_source.hpp"
#include "linux/linux_user_resolver.hpp"
#include "process/process_control.hpp"
#include "ui/process_killall_dialog.hpp"
#include "ui/root_view.hpp"
#include "ui/process_tab.hpp"
#include "ui/system_tab.hpp"

#include <cpptui.hpp>

#include <unistd.h>

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
    LinuxProcessSignalSender signalSender;
    ProcessControl processControl(
        procSource,
        signalSender,
        static_cast<ProcessId>(::getpid()));
    SnapshotCollector snapshotCollector(
        systemSource, filesystemCollector, processCollector);
    AppState state;
    CommandController controller(state, snapshotCollector);
    ProcessActionController processActionController(
        state, processControl);

    controller.Refresh();

    cpptui::App app;
    auto systemTab = std::make_shared<SystemTab>(state);
    auto processTab = std::make_shared<ProcessTab>(state);
    auto killallDialog = std::make_shared<ProcessKillallDialog>(
        app,
        [&processActionController, processTab](
            const ProcessKillallRequest& request)
        {
            processActionController.Execute(request);
            processTab->Update();
        });
    auto root = BuildRootView(state, systemTab, processTab);
    const auto openKillallDialog =
        [&state, processTab, killallDialog]()
        {
            if (state.Tab() != ApplicationTab::Processes ||
                killallDialog->IsOpen())
            {
                return;
            }

            const ProcessInfo* process =
                state.SelectedProcessInfo();
            if (!process)
            {
                state.SetProcessActionStatus(
                    {"No process selected", false});
                processTab->Update();
                return;
            }

            ProcessKillallRequest request;
            request.name = process->name;
            for (const auto& candidate :
                 state.Snapshot()->processes.processes)
            {
                if (candidate.name == process->name)
                {
                    request.identities.push_back(
                        candidate.identity);
                }
            }
            killallDialog->Open(request);
        };
    processTab->SetKillallAction(openKillallDialog);

    app.register_key(
        'q',
        [killallDialog]()
        {
            if (killallDialog->IsOpen())
            {
                killallDialog->Cancel();
                return;
            }

            cpptui::App::quit();
        });
    app.register_key(
        'r',
        [&controller, systemTab, processTab, killallDialog]()
        {
            if (killallDialog->IsOpen())
            {
                return;
            }
            controller.Refresh();
            systemTab->Update();
            processTab->Update();
        });
    app.register_key(
        'n',
        [&state, processTab, killallDialog]()
        {
            if (!killallDialog->IsOpen() &&
                state.Tab() == ApplicationTab::Processes)
            {
                state.SetSortKey(ProcessSortKey::Name);
                processTab->Update();
            }
        });
    app.register_key(
        'c',
        [&state, processTab, killallDialog]()
        {
            if (!killallDialog->IsOpen() &&
                state.Tab() == ApplicationTab::Processes)
            {
                state.SetSortKey(ProcessSortKey::Cpu);
                processTab->Update();
            }
        });
    app.register_key(
        'm',
        [&state, processTab, killallDialog]()
        {
            if (!killallDialog->IsOpen() &&
                state.Tab() == ApplicationTab::Processes)
            {
                state.SetSortKey(ProcessSortKey::Memory);
                processTab->Update();
            }
        });
    app.register_key(
        'k',
        [processTab]()
        {
            processTab->KillallSelected();
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
