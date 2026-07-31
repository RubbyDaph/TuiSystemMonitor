#include "ui/process_killall_dialog.hpp"

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

TEST_CASE("ProcessKillallDialog shows name and match count")
{
    cpptui::App app;
    std::vector<tsm::ProcessKillallRequest> confirmed;
    auto dialog = std::make_shared<tsm::ProcessKillallDialog>(
        app,
        [&confirmed](const tsm::ProcessKillallRequest& request)
        {
            confirmed.push_back(request);
        });

    const auto process = MakeDialogProcess();
    dialog->Open(
        {process.name,
         {process.identity, {124, 457}}});

    CHECK(dialog->IsOpen());
    CHECK(dialog->modal);
    REQUIRE(dialog->Request());
    CHECK(dialog->Request()->name == "worker");
    CHECK(dialog->Request()->identities.size() == 2);
    CHECK(dialog->PromptText().find("Terminate all") !=
          std::string::npos);
    CHECK(dialog->TargetText().find("Count: 2") !=
          std::string::npos);
    CHECK(dialog->WarningText().find("graceful") !=
          std::string::npos);
    CHECK(confirmed.empty());

    dialog->Cancel();
}

TEST_CASE("ProcessKillallDialog cancellation sends nothing")
{
    cpptui::App app;
    int confirmCount{};
    auto dialog = std::make_shared<tsm::ProcessKillallDialog>(
        app,
        [&confirmCount](const tsm::ProcessKillallRequest&)
        {
            ++confirmCount;
        });

    const auto process = MakeDialogProcess();
    const tsm::ProcessKillallRequest request{
        process.name, {process.identity}};
    dialog->Open(request);
    dialog->Cancel();

    CHECK_FALSE(dialog->IsOpen());
    CHECK_FALSE(dialog->Request());
    CHECK(confirmCount == 0);

    dialog->Open(request);
    cpptui::Event escape;
    escape.type = cpptui::EventType::Key;
    escape.key = 27;
    CHECK(dialog->on_event(escape));
    CHECK_FALSE(dialog->IsOpen());
    CHECK_FALSE(dialog->Request());
    CHECK(confirmCount == 0);

    dialog->Open(request);
    cpptui::Event quit;
    quit.type = cpptui::EventType::Key;
    quit.key = 'q';
    CHECK(dialog->on_event(quit));
    CHECK_FALSE(dialog->IsOpen());
    CHECK(confirmCount == 0);
}

TEST_CASE("ProcessKillallDialog confirms its captured group once")
{
    cpptui::App app;
    std::vector<tsm::ProcessKillallRequest> confirmed;
    auto dialog = std::make_shared<tsm::ProcessKillallDialog>(
        app,
        [&confirmed](const tsm::ProcessKillallRequest& request)
        {
            confirmed.push_back(request);
        });
    const auto process = MakeDialogProcess();
    tsm::ProcessKillallRequest request{
        process.name, {process.identity}};

    dialog->Open(request);
    request.identities = {{999, 888}};
    request.name = "changed";
    dialog->Confirm();
    dialog->Confirm();

    REQUIRE(confirmed.size() == 1);
    CHECK(confirmed[0].name == "worker");
    REQUIRE(confirmed[0].identities.size() == 1);
    CHECK(confirmed[0].identities[0] ==
          tsm::ProcessIdentity{123, 456});
    CHECK_FALSE(dialog->IsOpen());
    CHECK_FALSE(dialog->Request());
}
