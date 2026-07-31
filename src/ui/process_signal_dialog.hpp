#pragma once

#include "domain/process.hpp"

#include <cpptui.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace tsm
{

class ProcessSignalDialog final : public cpptui::Dialog
{
public:
    using ConfirmCallback =
        std::function<void(const ProcessSignalRequest&)>;

    ProcessSignalDialog(
        cpptui::App& app,
        ConfirmCallback onConfirm);

    void Open(const ProcessInfo& process);
    void Confirm();
    void Cancel();

    bool IsOpen() const;
    const std::optional<ProcessSignalRequest>& Request() const;
    const std::string& PromptText() const;
    const std::string& WarningText() const;

    bool on_event(const cpptui::Event& event) override;

private:
    ConfirmCallback onConfirm;
    std::optional<ProcessSignalRequest> request;
    std::shared_ptr<cpptui::Label> promptLabel;
    std::shared_ptr<cpptui::Label> targetLabel;
    std::shared_ptr<cpptui::Label> warningLabel;
    std::shared_ptr<cpptui::Button> cancelButton;
    std::shared_ptr<cpptui::Button> confirmButton;
};

}  // namespace tsm
