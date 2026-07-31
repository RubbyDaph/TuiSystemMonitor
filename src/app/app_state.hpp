#pragma once

#include "domain/process.hpp"
#include "domain/snapshot.hpp"

#include <optional>
#include <string>

namespace tsm
{

enum class ApplicationTab
{
    System,
    Processes,
};

struct ProcessActionStatus
{
    std::string message;
    bool success{};
};

class AppState
{
public:
    void ApplySnapshot(ApplicationSnapshot snapshot);
    void SetTab(ApplicationTab tab);
    void SetSortKey(ProcessSortKey key);
    void SetSelectedProcess(std::optional<ProcessIdentity> identity);
    void SetRefreshing(bool refreshing);
    void SetProcessActionStatus(ProcessActionStatus status);

    const std::optional<ApplicationSnapshot>& Snapshot() const;
    ApplicationTab Tab() const;
    ProcessSortKey SortKey() const;
    const std::optional<ProcessIdentity>& SelectedProcess() const;
    const ProcessInfo* SelectedProcessInfo() const;
    const std::string& StatusMessage() const;
    const std::optional<ProcessActionStatus>&
    ProcessStatus() const;
    bool IsRefreshing() const;

private:
    void SortProcesses();
    void RestoreSelection();

    std::optional<ApplicationSnapshot> snapshot;
    ApplicationTab tab{ApplicationTab::System};
    ProcessSortKey sortKey{ProcessSortKey::Cpu};
    std::optional<ProcessIdentity> selectedProcess;
    std::optional<ProcessActionStatus> processStatus;
    std::string statusMessage{"Not updated yet"};
    bool refreshing{};
};

}  // namespace tsm
