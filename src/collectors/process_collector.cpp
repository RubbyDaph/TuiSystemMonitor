#include "collectors/process_collector.hpp"

#include "parsers/process_stat_parser.hpp"
#include "parsers/process_status_parser.hpp"

namespace tsm
{

ProcessCollector::ProcessCollector(
    const ProcSource& procSource,
    const UserResolver& userResolver)
    : procSource(procSource),
      userResolver(userResolver)
{
}

ProcessCollection ProcessCollector::Collect() const
{
    ProcessCollection collection;
    auto processIds = procSource.ListProcessIds();
    if (!processIds)
    {
        collection.errors.push_back(processIds.GetError());
        collection.available = false;
        return collection;
    }

    for (const ProcessId pid : processIds.Value())
    {
        auto statText = procSource.ReadProcessStat(pid);
        if (!statText)
        {
            collection.errors.push_back(statText.GetError());
            continue;
        }

        auto stat = ProcessStatParser{}.Parse(statText.Value());
        if (!stat)
        {
            collection.errors.push_back(stat.GetError());
            continue;
        }

        auto statusText = procSource.ReadProcessStatus(pid);
        if (!statusText)
        {
            collection.errors.push_back(statusText.GetError());
            continue;
        }

        auto status = ProcessStatusParser{}.Parse(statusText.Value());
        if (!status)
        {
            collection.errors.push_back(status.GetError());
            continue;
        }

        auto user = userResolver.Resolve(status.Value().effectiveUid);
        if (!user)
        {
            collection.errors.push_back(user.GetError());
            continue;
        }

        collection.processes.push_back(
            {stat.Value().identity,
             stat.Value().name,
             status.Value().effectiveUid,
             user.Value(),
             stat.Value().state,
             stat.Value().userTicks,
             stat.Value().systemTicks,
             status.Value().residentMemoryBytes});
    }

    return collection;
}

}  // namespace tsm
