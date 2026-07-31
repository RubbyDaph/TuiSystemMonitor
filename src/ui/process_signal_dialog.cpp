#include "ui/process_signal_dialog.hpp"

#include <string>
#include <utility>

namespace tsm
{

ProcessSignalDialog::ProcessSignalDialog(
    cpptui::App& app,
    ConfirmCallback onConfirm)
    : cpptui::Dialog(
          &app,
          cpptui::BorderStyle::Rounded),
      onConfirm(std::move(onConfirm)),
      promptLabel(std::make_shared<cpptui::Label>("")),
      targetLabel(std::make_shared<cpptui::Label>("")),
      warningLabel(std::make_shared<cpptui::Label>("")),
      cancelButton(std::make_shared<cpptui::Button>(
          "Cancel",
          [this]()
          {
              Cancel();
          })),
      confirmButton(std::make_shared<cpptui::Button>(
          "Confirm",
          [this]()
          {
              Confirm();
          }))
{
    modal = true;
    fixed_width = 52;
    fixed_height = 9;
    width = fixed_width;
    height = fixed_height;

    promptLabel->fixed_height = 1;
    promptLabel->focusable = false;
    targetLabel->fixed_height = 1;
    targetLabel->focusable = false;
    warningLabel->fixed_height = 1;
    warningLabel->focusable = false;

    auto buttons = std::make_shared<cpptui::Horizontal>();
    buttons->fixed_height = 1;
    cancelButton->fixed_width = 16;
    confirmButton->fixed_width = 20;
    buttons->add(cancelButton);
    buttons->add(std::make_shared<cpptui::HorizontalSpacer>());
    buttons->add(confirmButton);

    auto content = std::make_shared<cpptui::Vertical>();
    content->add(promptLabel);
    content->add(targetLabel);
    content->add(warningLabel);
    content->add(std::make_shared<cpptui::VerticalSpacer>());
    content->add(buttons);
    add(content);
}

void ProcessSignalDialog::Open(
    const ProcessInfo& process)
{
    if (is_open)
    {
        return;
    }

    request = ProcessSignalRequest{
        process.identity,
        process.name};

    set_title("Confirm termination");
    promptLabel->set_text(
        "Terminate the selected process?");
    targetLabel->set_text(
        "Name: " + process.name);
    warningLabel->set_text(
        "The process may perform a graceful shutdown.");
    confirmButton->set_label("Terminate");
    open();
}

void ProcessSignalDialog::Confirm()
{
    if (!request)
    {
        return;
    }

    const ProcessSignalRequest confirmed = *request;
    request.reset();
    close();
    if (onConfirm)
    {
        onConfirm(confirmed);
    }
}

void ProcessSignalDialog::Cancel()
{
    request.reset();
    close();
}

bool ProcessSignalDialog::IsOpen() const
{
    return is_open;
}

const std::optional<ProcessSignalRequest>&
ProcessSignalDialog::Request() const
{
    return request;
}

const std::string& ProcessSignalDialog::PromptText() const
{
    return promptLabel->get_text();
}

const std::string& ProcessSignalDialog::WarningText() const
{
    return warningLabel->get_text();
}

bool ProcessSignalDialog::on_event(
    const cpptui::Event& event)
{
    if (event.is_key_event() && event.is_escape())
    {
        Cancel();
        return true;
    }
    if (event.is_key_event() && event.key == 'q')
    {
        Cancel();
        return true;
    }

    return cpptui::Dialog::on_event(event);
}

}  // namespace tsm
