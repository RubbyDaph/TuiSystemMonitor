#include "app/command_controller.hpp"

namespace tsm
{

CommandController::CommandController(
    AppState& state,
    SnapshotCollector& collector)
    : state(state),
      collector(collector)
{
}

void CommandController::Refresh()
{
    state.SetRefreshing(true);
    state.ApplySnapshot(collector.Collect());
    state.SetRefreshing(false);
}

}  // namespace tsm
