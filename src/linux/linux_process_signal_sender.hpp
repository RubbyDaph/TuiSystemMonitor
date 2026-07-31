#pragma once

#include "process/process_control.hpp"

namespace tsm
{

class LinuxProcessSignalSender final : public ProcessSignalSender
{
public:
    std::error_code Send(ProcessId pid) const override;
};

}  // namespace tsm
