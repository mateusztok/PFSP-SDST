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

    struct OpRec
    {
        int job;
        long long start;
        long long end;
        int setup_used;
    };

public:
    NEHWithProgress(const Instance &inst) : instance(inst) {}

    Schedule solve(bool verbose = true)
    {
        int n = instance.getJobs();
        int m = instance.getMachines();

        if (verbose)
        {
            std::cout << "Starting NEH with Progress Logging..." << std::endl;
        }

        // Sortowanie zadań według malejącego czasu całkowitego
        std::vector<std::pair<long long, int>> jobKeys;
        for (int j = 0; j < n; ++j)
        {
            long long totalTime = 0;
            for (int mach = 0; mach < m; ++mach)
            {
                totalTime += instance.getProcTime(j, mach);
            }
            jobKeys.push_back({-totalTime, j}); // ujemny dla sortowania malejącego
        }
        std::sort(jobKeys.begin(), jobKeys.end());

        std::vector<int> sequence;
        long long lastMakespan = -1;

        // Dodaj pierwsze zadanie
        if (!jobKeys.empty())
        {
            sequence.push_back(jobKeys[0].second);
            long long c0 = computeMakespan(sequence);
            lastMakespan = c0;

            if (verbose)
            {
                logProgress(1, sequence, c0, 0);
                streamSchedule(sequence, 1);
            }
        }

        // Dodawaj kolejne zadania
        for (size_t t = 1; t < jobKeys.size(); ++t)
        {
            int newJob = jobKeys[t].second;
            insertBestWithChoice(sequence, newJob, t + 1, verbose);

            long long currentMakespan = computeMakespan(sequence);
            long long delta = (lastMakespan < 0 ? 0 : lastMakespan - currentMakespan);
            lastMakespan = currentMakespan;

            if (verbose)
            {
                logProgress((int)sequence.size(), sequence, currentMakespan, delta);
                streamSchedule(sequence, (int)sequence.size());
            }
        }

        if (verbose)
        {
            std::cout << "NEH_RESULT;perm=";
            for (size_t i = 0; i < sequence.size(); ++i)
            {
                std::cout << (i ? "," : "") << sequence[i];
            }
            std::cout << ";cmax=" << lastMakespan << std::endl;
            std::cout << "END" << std::endl;
        }

        return Schedule(sequence);
    }

private:
    void insertBestWithChoice(std::vector<int> &sequence, int job, int iteration, bool verbose)
    {
        std::vector<int> bestSeq;
        double bestMakespan = 1e9;
        int bestPos = -1;

        // Przetestuj wszystkie pozycje
        for (int pos = 0; pos <= (int)sequence.size(); ++pos)
        {
            std::vector<int> candidate = sequence;
            candidate.insert(candidate.begin() + pos, job);

            double makespan = computeMakespan(candidate);
            if (makespan < bestMakespan)
            {
                bestMakespan = makespan;
                bestSeq = candidate;
                bestPos = pos;
            }
        }

        sequence = bestSeq;

        if (verbose)
        {
            std::cout << "NEH_CHOICE;iter=" << iteration
                      << ";job=" << job
                      << ";best_pos=" << bestPos
                      << ";cmax_after=" << bestMakespan << std::endl;
        }
    }

    void logProgress(int iteration, const std::vector<int> &seq, long long makespan, long long delta)
    {
        int n = instance.getJobs();
        double percent = double(iteration) / std::max(1, n) * 100.0;

        std::cout << "NEH_PROGRESS;iter=" << iteration
                  << ";percent=" << std::fixed << std::setprecision(1) << percent
                  << ";cmax=" << makespan
                  << ";delta=" << delta
                  << ";seq=";

        for (size_t i = 0; i < seq.size(); ++i)
        {
            std::cout << (i ? "," : "") << seq[i];
        }
        std::cout << std::endl;
    }

    void streamSchedule(const std::vector<int> &sequence, int iteration)
    {
        if (sequence.empty())
            return;

        int m = instance.getMachines();
        int n = sequence.size();

        // Buduj harmonogram
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

        // Wypisz sloty
        for (int m = 0; m < (int)schedule.size(); ++m)
        {
            int prevJob = -1;
            for (const auto &op : schedule[m])
            {
                std::cout << "SLOT;iter=" << iteration
                          << ";machine=" << m
                          << ";prev=" << (prevJob < 0 ? -1 : prevJob)
                          << ";job=" << op.job
                          << ";setup=" << op.setup_used
                          << ";start=" << op.start
                          << ";end=" << op.end << std::endl;
                prevJob = op.job;
            }
        }
        std::cout << "FRAME_END;iter=" << iteration << std::endl;
    }

    double computeMakespan(const std::vector<int> &sequence)
    {
        return instance.computeMakespan(sequence);
    }
};
