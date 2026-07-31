#pragma once

#include "app/app_state.hpp"

#include <cpptui.hpp>

#include <cstddef>
#include <memory>
#include <string>

namespace tsm
{

class ProcessTable;

class ProcessTab final : public cpptui::Vertical
{
public:
    explicit ProcessTab(AppState& state);

    void Update();
    void SelectRow(std::size_t index);

    std::size_t RowCount() const;
    int SelectedRow() const;
    const std::string& StatusText() const;
    std::string CellText(
        std::size_t row,
        std::size_t column) const;

private:
    void ApplyTableSelection(int index);
    std::string SortName() const;

    AppState& state;
    std::shared_ptr<cpptui::Label> statusLabel;
    std::shared_ptr<ProcessTable> processTable;
};

}  // namespace tsm
