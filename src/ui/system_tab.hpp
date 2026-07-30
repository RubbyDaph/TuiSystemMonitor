#pragma once

#include "app/app_state.hpp"

#include <cpptui.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace tsm
{

class SystemTab final : public cpptui::Vertical
{
public:
    explicit SystemTab(const AppState& state);

    void Update();

    const std::string& CpuText() const;
    const std::string& MemoryText() const;
    const std::string& SwapText() const;
    std::size_t FilesystemRowCount() const;
    std::size_t CpuBarCount() const;
    float CpuBarValue(std::size_t index) const;
    const std::string& CpuPercentText(
        std::size_t index) const;
    float MemoryBarValue() const;
    const std::string& MemoryPercentText() const;

private:
    const AppState& state;
    std::shared_ptr<cpptui::Label> statusLabel;
    std::shared_ptr<cpptui::Label> cpuLabel;
    std::vector<std::shared_ptr<cpptui::Label>> cpuPercentLabels;
    std::vector<std::shared_ptr<cpptui::ProgressBar>> cpuBars;
    std::shared_ptr<cpptui::Label> memoryLabel;
    std::shared_ptr<cpptui::Label> memoryPercentLabel;
    std::shared_ptr<cpptui::ProgressBar> memoryBar;
    std::shared_ptr<cpptui::Label> swapLabel;
    std::shared_ptr<cpptui::TableScrollable> filesystemTable;
};

}  // namespace tsm
