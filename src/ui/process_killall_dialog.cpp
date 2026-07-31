#include "ui/process_killall_dialog.hpp"

#include <string>
#include <utility>

namespace tsm
{

ProcessKillallDialog::ProcessKillallDialog(
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

void ProcessKillallDialog::Open(
    const ProcessKillallRequest& newRequest)
{
    if (is_open)
    {
        return;
    }

    if (newRequest.identities.empty())
    {
        return;
    }
    request = newRequest;

    set_title("Confirm killall");
    promptLabel->set_text(
        "Terminate all processes with this name?");
    targetLabel->set_text(
        "Name: " + request->name + " | Count: " +
        std::to_string(request->identities.size()));
    warningLabel->set_text(
        "The process may perform a graceful shutdown.");
    confirmButton->set_label("Killall");
    open();
}

void ProcessKillallDialog::Confirm()
{
    if (!request)
    {
        return;
    }

    const ProcessKillallRequest confirmed = *request;
    request.reset();
    close();
    if (onConfirm)
    {
        onConfirm(confirmed);
    }
}

void ProcessKillallDialog::Cancel()
{
    request.reset();
    close();
}

bool ProcessKillallDialog::IsOpen() const
{
    return is_open;
}

const std::optional<ProcessKillallRequest>&
ProcessKillallDialog::Request() const
{
    return request;
}

const std::string& ProcessKillallDialog::PromptText() const
{
    return promptLabel->get_text();
}

const std::string& ProcessKillallDialog::TargetText() const
{
    return targetLabel->get_text();
}

const std::string& ProcessKillallDialog::WarningText() const
{
    return warningLabel->get_text();
}

bool ProcessKillallDialog::on_event(
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
