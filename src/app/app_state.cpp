#include "app/app_state.hpp"

#include "metrics/process_sorter.hpp"

#include <algorithm>
#include <utility>

namespace tsm
{
namespace
{

template <typename T>
void MergeSection(MetricSection<T>& current, MetricSection<T> incoming)
{
    if (incoming.data)
    {
        current = std::move(incoming);
        current.stale = false;
        return;
    }

    if (current.data)
    {
        current.errors = std::move(incoming.errors);
        current.stale = true;
        return;
    }

    current = std::move(incoming);
}

bool HasErrors(const ApplicationSnapshot& snapshot)
{
    return !snapshot.system.cpu.errors.empty() ||
           !snapshot.system.memory.errors.empty() ||
           !snapshot.system.filesystems.errors.empty() ||
           !snapshot.processes.available;
}

}  // namespace

void AppState::ApplySnapshot(ApplicationSnapshot incoming)
{
    const bool hasErrors = HasErrors(incoming);
    if (!snapshot)
    {
        snapshot = std::move(incoming);
    }
    else
    {
        snapshot->collectedAt = incoming.collectedAt;
        MergeSection(
            snapshot->system.cpu,
            std::move(incoming.system.cpu));
        MergeSection(
            snapshot->system.memory,
            std::move(incoming.system.memory));
        MergeSection(
            snapshot->system.filesystems,
            std::move(incoming.system.filesystems));

        if (incoming.processes.available)
        {
            snapshot->processes = std::move(incoming.processes);
            snapshot->processes.stale = false;
        }
        else
        {
            snapshot->processes.errors =
                std::move(incoming.processes.errors);
            snapshot->processes.available = false;
            snapshot->processes.stale = true;
        }
    }

    SortProcesses();
    RestoreSelection();
    statusMessage =
        hasErrors ? "Updated with errors" : "Updated successfully";
}

void AppState::SetTab(ApplicationTab newTab)
{
    tab = newTab;
}

void AppState::SetSortKey(ProcessSortKey newSortKey)
{
    sortKey = newSortKey;
    SortProcesses();
    RestoreSelection();
}

void AppState::SetSelectedProcess(
    std::optional<ProcessIdentity> identity)
{
    selectedProcess = identity;
    RestoreSelection();
}

void AppState::SetRefreshing(bool newRefreshing)
{
    refreshing = newRefreshing;
}

void AppState::SetProcessActionStatus(ProcessActionStatus status)
{
    processStatus = std::move(status);
}

const std::optional<ApplicationSnapshot>& AppState::Snapshot() const
{
    return snapshot;
}

ApplicationTab AppState::Tab() const
{
    return tab;
}

ProcessSortKey AppState::SortKey() const
{
    return sortKey;
}

const std::optional<ProcessIdentity>& AppState::SelectedProcess() const
{
    return selectedProcess;
}

const ProcessInfo* AppState::SelectedProcessInfo() const
{
    if (!snapshot || !selectedProcess)
    {
        return nullptr;
    }

    const auto selected = std::find_if(
        snapshot->processes.processes.begin(),
        snapshot->processes.processes.end(),
        [this](const ProcessInfo& process)
        {
            return process.identity == *selectedProcess;
        });
    return selected == snapshot->processes.processes.end()
        ? nullptr
        : &*selected;
}

const std::string& AppState::StatusMessage() const
{
    return statusMessage;
}

const std::optional<ProcessActionStatus>&
AppState::ProcessStatus() const
{
    return processStatus;
}

bool AppState::IsRefreshing() const
{
    return refreshing;
}

void AppState::SortProcesses()
{
    if (snapshot)
    {
        ProcessSorter{}.Sort(
            snapshot->processes.processes, sortKey);
    }
}

void AppState::RestoreSelection()
{
    if (!snapshot || snapshot->processes.processes.empty())
    {
        selectedProcess.reset();
        return;
    }

    if (selectedProcess)
    {
        const auto selected = std::find_if(
            snapshot->processes.processes.begin(),
            snapshot->processes.processes.end(),
            [this](const ProcessInfo& process)
            {
                return process.identity == *selectedProcess;
            });
        if (selected != snapshot->processes.processes.end())
        {
            return;
        }
    }

    selectedProcess =
        snapshot->processes.processes.front().identity;
}

}  // namespace tsm
