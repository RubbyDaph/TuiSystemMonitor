#pragma once

#include "domain/process.hpp"
#include "linux/linux_proc_source.hpp"
#include "linux/linux_user_resolver.hpp"

namespace tsm
{

class ProcessCollector
{
public:
    ProcessCollector(
        const ProcSource& procSource,
        const UserResolver& userResolver);

    ProcessCollection Collect() const;

private:
    const ProcSource& procSource;
    const UserResolver& userResolver;
};

}  // namespace tsm
