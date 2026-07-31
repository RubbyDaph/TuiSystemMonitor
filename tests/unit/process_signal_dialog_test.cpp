#include "ui/process_signal_dialog.hpp"

#include <cpptui.hpp>

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <vector>

namespace
{

tsm::ProcessInfo MakeDialogProcess()
{
    return {
        {123, 456},
        "worker",
        "alice",
        tsm::ProcessState::Running,
        10.0,
        1024};
}

}  // namespace

TEST_CASE("ProcessSignalDialog shows the selected target")
{
    cpptui::App app;
    std::vector<tsm::ProcessSignalRequest> confirmed;
    auto dialog = std::make_shared<tsm::ProcessSignalDialog>(
        app,
        [&confirmed](const tsm::ProcessSignalRequest& request)
        {
            confirmed.push_back(request);
        });

    dialog->Open(MakeDialogProcess());

    CHECK(dialog->IsOpen());
    CHECK(dialog->modal);
    REQUIRE(dialog->Request());
    CHECK(dialog->Request()->identity ==
          tsm::ProcessIdentity{123, 456});
    CHECK(dialog->Request()->name == "worker");
    CHECK(dialog->PromptText().find("Terminate") !=
          std::string::npos);
    CHECK(dialog->WarningText().find("graceful") !=
          std::string::npos);
    CHECK(confirmed.empty());

    dialog->Cancel();
}

TEST_CASE("ProcessSignalDialog cancellation sends nothing")
{
    cpptui::App app;
    int confirmCount{};
    auto dialog = std::make_shared<tsm::ProcessSignalDialog>(
        app,
        [&confirmCount](const tsm::ProcessSignalRequest&)
        {
            ++confirmCount;
        });

    dialog->Open(MakeDialogProcess());
    dialog->Cancel();

    CHECK_FALSE(dialog->IsOpen());
    CHECK_FALSE(dialog->Request());
    CHECK(confirmCount == 0);

    dialog->Open(MakeDialogProcess());
    cpptui::Event escape;
    escape.type = cpptui::EventType::Key;
    escape.key = 27;
    CHECK(dialog->on_event(escape));
    CHECK_FALSE(dialog->IsOpen());
    CHECK_FALSE(dialog->Request());
    CHECK(confirmCount == 0);

    dialog->Open(MakeDialogProcess());
    cpptui::Event quit;
    quit.type = cpptui::EventType::Key;
    quit.key = 'q';
    CHECK(dialog->on_event(quit));
    CHECK_FALSE(dialog->IsOpen());
    CHECK(confirmCount == 0);
}

TEST_CASE("ProcessSignalDialog confirms its captured target once")
{
    cpptui::App app;
    std::vector<tsm::ProcessSignalRequest> confirmed;
    auto dialog = std::make_shared<tsm::ProcessSignalDialog>(
        app,
        [&confirmed](const tsm::ProcessSignalRequest& request)
        {
            confirmed.push_back(request);
        });
    auto process = MakeDialogProcess();

    dialog->Open(process);
    process.identity = {999, 888};
    process.name = "changed";
    dialog->Confirm();
    dialog->Confirm();

    REQUIRE(confirmed.size() == 1);
    CHECK(confirmed[0].identity ==
          tsm::ProcessIdentity{123, 456});
    CHECK(confirmed[0].name == "worker");
    CHECK_FALSE(dialog->IsOpen());
    CHECK_FALSE(dialog->Request());
}
