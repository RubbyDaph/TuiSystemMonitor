#include "ui/process_tab.hpp"

#include "ui/formatters.hpp"

#include <algorithm>
#include <string>

namespace tsm
{

class ProcessTable final : public cpptui::TableScrollable
{
public:
    bool on_event(const cpptui::Event& event) override
    {
        if (rows.empty() && event.is_key_event() &&
            (event.is_nav_up() ||
             event.is_nav_down() ||
             event.is_view_scroll_up() ||
             event.is_view_scroll_down() ||
             event.is_nav_home() ||
             event.is_nav_end() ||
             event.is_enter()))
        {
            return true;
        }

        return cpptui::TableScrollable::on_event(event);
    }
};

ProcessTab::ProcessTab(AppState& state)
    : state(state),
      statusLabel(std::make_shared<cpptui::Label>("Waiting for data")),
      actionLabel(std::make_shared<cpptui::Label>(
          "No process action yet")),
      processTable(std::make_shared<ProcessTable>())
{
    statusLabel->fixed_height = 1;
    actionLabel->fixed_height = 1;
    processTable->columns = {
        "PID", "Name", "User", "CPU", "RAM", "State"};
    processTable->col_widths = {8, 24, 16, 10, 12, 12};
    processTable->on_change =
        [this](int index)
        {
            ApplyTableSelection(index);
        };

    add(statusLabel);
    add(actionLabel);
    add(processTable);
    Update();
}

void ProcessTab::Update()
{
    processTable->rows.clear();
    if (state.ProcessStatus())
    {
        actionLabel->set_text(
            std::string(
                state.ProcessStatus()->success
                    ? "Success: "
                    : "Error: ") +
            state.ProcessStatus()->message);
    }
    else
    {
        actionLabel->set_text("No process action yet");
    }

    if (!state.Snapshot())
    {
        statusLabel->set_text(
            "Waiting for data | r: refresh | p/c/m: sort | "
            "t: terminate | k: kill");
        processTable->selected_index = 0;
        processTable->scroll_offset = 0;
        return;
    }

    const auto& processes = state.Snapshot()->processes;
    for (const auto& process : processes.processes)
    {
        processTable->add_row(
            {std::to_string(process.identity.pid),
             process.name,
             process.user,
             FormatPercent(process.cpuPercent),
             FormatBytes(process.residentMemoryBytes),
             FormatProcessState(process.state)});
    }

    std::string availability = "Live";
    if (processes.stale || !processes.available)
    {
        availability = "Stale";
    }
    else if (!processes.errors.empty())
    {
        availability = "Live with skipped processes";
    }

    statusLabel->set_text(
        availability + " | Sort: " + SortName() +
        " | r: refresh | p/c/m: sort | t: terminate | k: kill");

    if (processes.processes.empty())
    {
        processTable->selected_index = 0;
        processTable->scroll_offset = 0;
        return;
    }

    int selectedIndex = 0;
    if (state.SelectedProcess())
    {
        const auto selected = std::find_if(
            processes.processes.begin(),
            processes.processes.end(),
            [this](const ProcessInfo& process)
            {
                return process.identity == *state.SelectedProcess();
            });
        if (selected != processes.processes.end())
        {
            selectedIndex = static_cast<int>(
                std::distance(processes.processes.begin(), selected));
        }
    }

    processTable->selected_index = selectedIndex;
    processTable->scroll_to_selection();
}

void ProcessTab::SelectRow(std::size_t index)
{
    if (!state.Snapshot() ||
        index >= state.Snapshot()->processes.processes.size())
    {
        return;
    }

    processTable->selected_index = static_cast<int>(index);
    processTable->scroll_to_selection();
    ApplyTableSelection(static_cast<int>(index));
}

std::size_t ProcessTab::RowCount() const
{
    return processTable->rows.size();
}

int ProcessTab::SelectedRow() const
{
    return processTable->selected_index;
}

const std::string& ProcessTab::StatusText() const
{
    return statusLabel->get_text();
}

const std::string& ProcessTab::ActionText() const
{
    return actionLabel->get_text();
}

std::string ProcessTab::CellText(
    std::size_t row,
    std::size_t column) const
{
    return processTable->rows.at(row).at(column).plain_text();
}

void ProcessTab::ApplyTableSelection(int index)
{
    if (!state.Snapshot() || index < 0 ||
        static_cast<std::size_t>(index) >=
            state.Snapshot()->processes.processes.size())
    {
        return;
    }

    state.SetSelectedProcess(
        state.Snapshot()->processes.processes[
            static_cast<std::size_t>(index)].identity);
}

std::string ProcessTab::SortName() const
{
    switch (state.SortKey())
    {
        case ProcessSortKey::Pid:
            return "PID";
        case ProcessSortKey::Cpu:
            return "CPU";
        case ProcessSortKey::Memory:
            return "RAM";
    }

    return "CPU";
}

}  // namespace tsm
