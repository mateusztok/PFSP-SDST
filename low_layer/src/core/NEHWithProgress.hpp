#pragma once
#include <vector>
#include <algorithm>
#include <iostream>
#include <climits>
#include <iomanip>
#include "Instance.hpp"
#include "Schedule.hpp"

class NEHWithProgress
{
private:
    const Instance &instance;

public:
    NEHWithProgress(const Instance &inst) : instance(inst) {}

    Schedule solve()
    {
        int n = instance.getJobs();
        int m = instance.getMachines();

        std::vector<std::pair<long long, int>> jobKeys;
        for (int j = 0; j < n; ++j)
        {
            long long totalTime = 0;
            for (int mach = 0; mach < m; ++mach)
            {
                totalTime += instance.getProcTime(j, mach);
            }
            jobKeys.push_back({-totalTime, j});
        }
        std::sort(jobKeys.begin(), jobKeys.end());

        std::vector<int> sequence;

        if (!jobKeys.empty())
        {
            sequence.push_back(jobKeys[0].second);
        }

        for (size_t t = 1; t < jobKeys.size(); ++t)
        {
            int newJob = jobKeys[t].second;
            insertBestWithChoice(sequence, newJob, t + 1);
        }

        Schedule(sequence).emitFinalSlots(instance);

        return Schedule(sequence);
    }

private:
    void insertBestWithChoice(std::vector<int> &sequence, int job, int iteration)
    {
        std::vector<int> bestSeq;
        double bestMakespan = 1e9;

        for (int pos = 0; pos <= (int)sequence.size(); ++pos)
        {
            std::vector<int> candidate = sequence;
            candidate.insert(candidate.begin() + pos, job);

            double makespan = computeMakespan(candidate);
            if (makespan < bestMakespan)
            {
                bestMakespan = makespan;
                bestSeq = candidate;
            }
        }

        sequence = bestSeq;
    }

    double computeMakespan(const std::vector<int> &sequence)
    {
        return instance.computeMakespan(sequence);
    }
};
