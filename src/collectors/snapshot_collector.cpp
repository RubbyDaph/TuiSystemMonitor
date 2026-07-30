#include "collectors/snapshot_collector.hpp"

#include "metrics/cpu_usage_calculator.hpp"
#include "metrics/process_usage_calculator.hpp"
#include "parsers/meminfo_parser.hpp"
#include "parsers/proc_stat_parser.hpp"

#include <chrono>
#include <sstream>
#include <utility>

namespace tsm
{

SnapshotCollector::SnapshotCollector(
    const SystemSource& systemSource,
    const FilesystemCollector& filesystemCollector,
    const ProcessCollector& processCollector)
    : systemSource(systemSource),
      filesystemCollector(filesystemCollector),
      processCollector(processCollector)
{
}

std::vector<ProcessInfo> SnapshotCollector::BuildUnknownProcesses(
    const std::vector<ProcessRawSample>& processes) const
{
    std::vector<ProcessInfo> result;
    result.reserve(processes.size());
    for (const auto& process : processes)
    {
        result.push_back(
            {process.identity,
             process.name,
             process.user,
             process.state,
             std::nullopt,
             process.residentMemoryBytes});
    }
    return result;
}

ApplicationSnapshot SnapshotCollector::Collect()
{
    ApplicationSnapshot snapshot;
    snapshot.collectedAt = std::chrono::steady_clock::now();

    std::optional<CpuSample> currentCpu;
    auto cpuText = systemSource.ReadCpuStat();
    if (!cpuText)
    {
        snapshot.system.cpu.errors.push_back(cpuText.GetError());
    }
    else
    {
        auto parsedCpu = ProcStatParser{}.Parse(cpuText.Value());
        if (!parsedCpu)
        {
            snapshot.system.cpu.errors.push_back(parsedCpu.GetError());
        }
        else
        {
            currentCpu = parsedCpu.Value();
            if (!previousSystemCpu)
            {
                CpuUsage usage;
                usage.corePercentages.resize(
                    currentCpu->cores.size(), std::nullopt);
                snapshot.system.cpu.data = std::move(usage);
            }
            else
            {
                auto usage = CpuUsageCalculator{}.Calculate(
                    *previousSystemCpu, *currentCpu);
                if (usage)
                {
                    snapshot.system.cpu.data =
                        std::move(usage.Value());
                }
                else
                {
                    snapshot.system.cpu.errors.push_back(
                        usage.GetError());
                }
            }
            previousSystemCpu = currentCpu;
        }
    }

    auto memoryText = systemSource.ReadMeminfo();
    if (!memoryText)
    {
        snapshot.system.memory.errors.push_back(
            memoryText.GetError());
    }
    else
    {
        auto memory = MeminfoParser{}.Parse(memoryText.Value());
        if (memory)
        {
            snapshot.system.memory.data = std::move(memory.Value());
        }
        else
        {
            snapshot.system.memory.errors.push_back(
                memory.GetError());
        }
    }

    auto mountinfoText = systemSource.ReadMountinfo();
    if (!mountinfoText)
    {
        snapshot.system.filesystems.errors.push_back(
            mountinfoText.GetError());
    }
    else
    {
        std::istringstream mountinfo(mountinfoText.Value());
        auto filesystems = filesystemCollector.Collect(mountinfo);
        if (filesystems.available)
        {
            snapshot.system.filesystems.data =
                std::move(filesystems.filesystems);
        }
        snapshot.system.filesystems.errors =
            std::move(filesystems.errors);
    }

    auto processes = processCollector.Collect();
    snapshot.processes.available = processes.available;
    snapshot.processes.errors = processes.errors;
    if (processes.available)
    {
        snapshot.processes.processes =
            BuildUnknownProcesses(processes.processes);

        if (currentCpu && previousProcessCpu)
        {
            auto calculated = ProcessUsageCalculator{}.Calculate(
                previousProcesses,
                processes.processes,
                *previousProcessCpu,
                *currentCpu);
            if (calculated)
            {
                snapshot.processes.processes =
                    std::move(calculated.Value());
            }
            else
            {
                snapshot.processes.errors.push_back(
                    calculated.GetError());
            }
        }

        if (currentCpu)
        {
            previousProcessCpu = currentCpu;
            previousProcesses = processes.processes;
        }
    }

    return snapshot;
}

}  // namespace tsm
