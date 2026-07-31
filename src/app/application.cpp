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
#include "ui/process_signal_dialog.hpp"
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
    auto signalDialog = std::make_shared<ProcessSignalDialog>(
        app,
        [&processActionController, processTab](
            const ProcessSignalRequest& request)
        {
            processActionController.Execute(request);
            processTab->Update();
        });
    auto root = BuildRootView(state, systemTab, processTab);
    const auto openSignalDialog =
        [&state, processTab, signalDialog](
            ProcessSignal signal)
        {
            if (state.Tab() != ApplicationTab::Processes ||
                signalDialog->IsOpen())
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

            signalDialog->Open(*process, signal);
        };

    app.register_key(
        'q',
        [signalDialog]()
        {
            if (signalDialog->IsOpen())
            {
                signalDialog->Cancel();
                return;
            }

            cpptui::App::quit();
        });
    app.register_key(
        'r',
        [&controller, systemTab, processTab, signalDialog]()
        {
            if (signalDialog->IsOpen())
            {
                return;
            }
            controller.Refresh();
            systemTab->Update();
            processTab->Update();
        });
    app.register_key(
        'p',
        [&state, processTab, signalDialog]()
        {
            if (!signalDialog->IsOpen() &&
                state.Tab() == ApplicationTab::Processes)
            {
                state.SetSortKey(ProcessSortKey::Pid);
                processTab->Update();
            }
        });
    app.register_key(
        'c',
        [&state, processTab, signalDialog]()
        {
            if (!signalDialog->IsOpen() &&
                state.Tab() == ApplicationTab::Processes)
            {
                state.SetSortKey(ProcessSortKey::Cpu);
                processTab->Update();
            }
        });
    app.register_key(
        'm',
        [&state, processTab, signalDialog]()
        {
            if (!signalDialog->IsOpen() &&
                state.Tab() == ApplicationTab::Processes)
            {
                state.SetSortKey(ProcessSortKey::Memory);
                processTab->Update();
            }
        });
    app.register_key(
        't',
        [openSignalDialog]()
        {
            openSignalDialog(ProcessSignal::Terminate);
        });
    app.register_key(
        'k',
        [openSignalDialog]()
        {
            openSignalDialog(ProcessSignal::Kill);
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
