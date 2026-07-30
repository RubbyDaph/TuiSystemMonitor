#pragma once

#include "app/app_state.hpp"
#include "collectors/snapshot_collector.hpp"

namespace tsm
{

class CommandController
{
public:
    CommandController(
        AppState& state,
        SnapshotCollector& collector);

    void Refresh();

private:
    AppState& state;
    SnapshotCollector& collector;
};

}  // namespace tsm
