#pragma once

#include "app/app_state.hpp"
#include "domain/process.hpp"
#include "process/process_control.hpp"

namespace tsm
{

class ProcessActionController
{
public:
    ProcessActionController(
        AppState& state,
        const ProcessControl& processControl);

    bool Execute(const ProcessSignalRequest& request);

private:
    AppState& state;
    const ProcessControl& processControl;
};

}  // namespace tsm
