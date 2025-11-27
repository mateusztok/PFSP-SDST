#include "Schedule.hpp"
#include "Instance.hpp"
#include <algorithm>

void Schedule::emitFinalSlots(const Instance &instance) const
{
    const std::vector<int> &sequence = jobSequence;
    if (sequence.empty())
        return;

    int m = instance.getMachines();
    int n = (int)sequence.size();

    struct OpRec
    {
        int job;
        long long start;
        long long end;
        int setup_used;
    };

    std::vector<std::vector<OpRec>> schedule(m);
    std::vector<long long> machineTime(m, 0);

    for (int idx = 0; idx < n; ++idx)
    {
        int job = sequence[idx];

        for (int mach = 0; mach < m; ++mach)
        {
            long long readyByMachine = (mach == 0) ? 0 : machineTime[mach - 1];
            long long readyBySequence = machineTime[mach];

            int prevJob = (idx == 0) ? -1 : sequence[idx - 1];
            int setupTime = (idx == 0 || prevJob == -1) ? 0 : instance.getSetupTime(mach, prevJob, job);

            long long startTime = std::max(readyByMachine, readyBySequence + setupTime);
            long long endTime = startTime + instance.getProcTime(job, mach);

            machineTime[mach] = endTime;
            schedule[mach].push_back({job, startTime, endTime, setupTime});
        }
    }

    for (int mIdx = 0; mIdx < (int)schedule.size(); ++mIdx)
    {
        int prevJob = -1;
        for (const auto &op : schedule[mIdx])
        {
            std::cout << "SLOT;"
                      << ";machine=" << mIdx
                      << ";prev=" << (prevJob < 0 ? -1 : prevJob)
                      << ";job=" << op.job
                      << ";setup=" << op.setup_used
                      << ";start=" << op.start
                      << ";end=" << op.end << std::endl;
            prevJob = op.job;
        }
    }
}
