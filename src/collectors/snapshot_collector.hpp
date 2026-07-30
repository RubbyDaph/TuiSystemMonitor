#pragma once

#include "collectors/filesystem_collector.hpp"
#include "collectors/process_collector.hpp"
#include "domain/cpu.hpp"
#include "domain/process.hpp"
#include "domain/snapshot.hpp"
#include "linux/linux_system_source.hpp"

#include <optional>
#include <vector>

namespace tsm
{

class SnapshotCollector
{
public:
    SnapshotCollector(
        const SystemSource& systemSource,
        const FilesystemCollector& filesystemCollector,
        const ProcessCollector& processCollector);

    ApplicationSnapshot Collect();

private:
    std::vector<ProcessInfo> BuildUnknownProcesses(
        const std::vector<ProcessRawSample>& processes) const;

    const SystemSource& systemSource;
    const FilesystemCollector& filesystemCollector;
    const ProcessCollector& processCollector;
    std::optional<CpuSample> previousSystemCpu;
    std::optional<CpuSample> previousProcessCpu;
    std::vector<ProcessRawSample> previousProcesses;
};

}  // namespace tsm
