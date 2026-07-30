#include "ui/system_tab.hpp"

#include "ui/formatters.hpp"

#include <algorithm>
#include <string>
#include <unordered_set>

namespace tsm
{
namespace
{

std::string StaleSuffix(bool stale)
{
    return stale ? " [stale]" : "";
}

}  // namespace

SystemTab::SystemTab(const AppState& state)
    : state(state),
      statusLabel(std::make_shared<cpptui::Label>("Waiting for data")),
      cpuLabel(std::make_shared<cpptui::Label>("CPU load")),
      memoryLabel(std::make_shared<cpptui::Label>("RAM: -")),
      memoryPercentLabel(std::make_shared<cpptui::Label>("-")),
      memoryBar(std::make_shared<cpptui::ProgressBar>()),
      swapLabel(std::make_shared<cpptui::Label>("Swap: -")),
      filesystemTable(std::make_shared<cpptui::TableScrollable>())
{
    statusLabel->fixed_height = 1;
    cpuLabel->fixed_height = 1;
    memoryLabel->fixed_height = 1;
    memoryPercentLabel->fixed_width = 8;
    swapLabel->fixed_height = 1;
    filesystemTable->columns = {
        "Drive", "Total", "Used", "Free", "Usage"};

    add(statusLabel);
    add(cpuLabel);
    for (std::size_t index = 0; index < 4; ++index)
    {
        auto row = std::make_shared<cpptui::Horizontal>();
        row->fixed_height = 1;
        auto name = std::make_shared<cpptui::Label>(
            "CPU " + std::to_string(index + 1));
        name->fixed_width = 8;
        auto percent =
            std::make_shared<cpptui::Label>("-");
        percent->fixed_width = 8;
        auto bar = std::make_shared<cpptui::ProgressBar>();
        cpuPercentLabels.push_back(percent);
        cpuBars.push_back(bar);
        row->add(name);
        row->add(percent);
        row->add(bar);
        add(row);
    }
    add(memoryLabel);
    auto memoryRow = std::make_shared<cpptui::Horizontal>();
    memoryRow->fixed_height = 1;
    auto memoryName = std::make_shared<cpptui::Label>("RAM");
    memoryName->fixed_width = 8;
    memoryRow->add(memoryName);
    memoryRow->add(memoryPercentLabel);
    memoryRow->add(memoryBar);
    add(memoryRow);
    add(swapLabel);
    add(filesystemTable);
    Update();
}

void SystemTab::Update()
{
    const auto status =
        state.StatusMessage() == "Updated successfully"
            ? "Live"
            : state.StatusMessage();
    statusLabel->set_text(status + " | q: quit | [ ]: tabs");

    if (!state.Snapshot())
    {
        return;
    }

    const auto& snapshot = *state.Snapshot();
    if (snapshot.system.cpu.data)
    {
        const auto& cpu = *snapshot.system.cpu.data;
        cpuLabel->set_text(
            "CPU load" +
            StaleSuffix(snapshot.system.cpu.stale));

        for (std::size_t index = 0;
             index < cpuBars.size();
             ++index)
        {
            const auto percent =
                index < cpu.corePercentages.size() &&
                        cpu.corePercentages[index]
                    ? *cpu.corePercentages[index]
                    : 0.0;
            cpuBars[index]->value = static_cast<float>(
                std::clamp(percent / 100.0, 0.0, 1.0));
            cpuPercentLabels[index]->set_text(
                FormatPercent(percent));
        }
    }
    else
    {
        cpuLabel->set_text("CPU: unavailable");
        for (const auto& bar : cpuBars)
        {
            bar->value = 0.0F;
        }
        for (const auto& percent : cpuPercentLabels)
        {
            percent->set_text("-");
        }
    }

    if (snapshot.system.memory.data)
    {
        const auto& memory = *snapshot.system.memory.data;
        memoryLabel->set_text(
            "RAM:  " + FormatBytes(memory.ramUsedBytes) + " / " +
            FormatBytes(memory.ramTotalBytes) +
            StaleSuffix(snapshot.system.memory.stale));
        const auto memoryPercent =
            memory.ramTotalBytes == 0
                ? 0.0
                : 100.0 *
                      static_cast<double>(memory.ramUsedBytes) /
                      static_cast<double>(memory.ramTotalBytes);
        memoryBar->value = static_cast<float>(
            std::clamp(memoryPercent / 100.0, 0.0, 1.0));
        memoryPercentLabel->set_text(
            FormatPercent(memoryPercent));
        swapLabel->set_text(
            "Swap: " + FormatBytes(memory.swapUsedBytes) + " / " +
            FormatBytes(memory.swapTotalBytes) +
            StaleSuffix(snapshot.system.memory.stale));
    }
    else
    {
        memoryLabel->set_text("RAM: unavailable");
        memoryPercentLabel->set_text("-");
        memoryBar->value = 0.0F;
        swapLabel->set_text("Swap: unavailable");
    }

    filesystemTable->rows.clear();
    if (snapshot.system.filesystems.data)
    {
        std::unordered_set<std::string> displayedDrives;
        for (const auto& filesystem :
             *snapshot.system.filesystems.data)
        {
            if (!displayedDrives
                     .insert(filesystem.mount.source)
                     .second)
            {
                continue;
            }
            filesystemTable->add_row(
                {filesystem.mount.source,
                 FormatBytes(filesystem.totalBytes),
                 FormatBytes(filesystem.usedBytes),
                 FormatBytes(filesystem.availableBytes),
                 FormatPercent(filesystem.usedPercent)});
        }
    }
}

const std::string& SystemTab::CpuText() const
{
    return cpuLabel->get_text();
}

const std::string& SystemTab::MemoryText() const
{
    return memoryLabel->get_text();
}

const std::string& SystemTab::SwapText() const
{
    return swapLabel->get_text();
}

std::size_t SystemTab::CpuBarCount() const
{
    return cpuBars.size();
}

std::size_t SystemTab::FilesystemRowCount() const
{
    return filesystemTable->rows.size();
}

float SystemTab::CpuBarValue(std::size_t index) const
{
    return cpuBars.at(index)->value;
}

const std::string& SystemTab::CpuPercentText(
    std::size_t index) const
{
    return cpuPercentLabels.at(index)->get_text();
}

float SystemTab::MemoryBarValue() const
{
    return memoryBar->value;
}

const std::string& SystemTab::MemoryPercentText() const
{
    return memoryPercentLabel->get_text();
}

}  // namespace tsm
