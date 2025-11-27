#pragma once
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <iostream>
#include "Instance.hpp"
#include "Schedule.hpp"

class SimulatedAnnealing
{
private:
    const Instance &instance;
    std::mt19937 rng;

    int maxIterations;
    double initialTemperature;
    double coolingFactor;

public:
    SimulatedAnnealing(const Instance &inst, int seed = 0)
        : instance(inst), rng(seed)
    {

        maxIterations = 50000;
        initialTemperature = 100.0;
        coolingFactor = 0.9975;
    }

    void setParameters(int iterations, double temp, double cooling)
    {
        maxIterations = iterations;
        initialTemperature = temp;
        coolingFactor = cooling;
    }

    Schedule solve(const Schedule &initialSolution = Schedule())
    {
        auto currentSeq = makeStartSequence(initialSolution);
        return runSAFromSeq(currentSeq, maxIterations, initialTemperature, coolingFactor, "SIMULATED_ANNEALING");
    }

private:
    std::vector<int> makeStartSequence(const Schedule &initialSolution)
    {
        int n = instance.getJobs();
        std::vector<int> seq;
        if (initialSolution.getJobSequence().empty())
        {
            seq.resize(n);
            for (int i = 0; i < n; ++i)
            {
                seq[i] = i;
            }
            std::shuffle(seq.begin(), seq.end(), rng);
        }
        else
        {
            seq = initialSolution.getJobSequence();
        }
        return seq;
    }

    Schedule runSAFromSeq(std::vector<int> startSeq, int iterations, double initTemp, double cooling, const std::string &prefix)
    {
        int n = instance.getJobs();
        double currentCmax = instance.computeMakespan(startSeq);
        std::vector<int> bestSeq = startSeq;
        double bestCmax = currentCmax;

        double temperature = initTemp;
        std::uniform_int_distribution<int> jobDist(0, n - 1);
        std::uniform_real_distribution<double> probDist(0.0, 1.0);

        std::cout << "Starting " << prefix << "..." << std::endl;
        std::cout << "Initial makespan: " << currentCmax << std::endl;

        for (int iteration = 0; iteration < iterations; iteration++)
        {
            int pos1 = jobDist(rng);
            int pos2 = jobDist(rng);

            std::swap(startSeq[pos1], startSeq[pos2]);

            double newCmax = instance.computeMakespan(startSeq);
            double delta = newCmax - currentCmax;

            bool accept = false;
            if (delta < 0)
            {
                accept = true;
            }
            else
            {
                double probability = std::exp(-delta / temperature);
                accept = (probDist(rng) < probability);
            }

            if (accept)
            {
                currentCmax = newCmax;
                if (newCmax < bestCmax)
                {
                    bestCmax = newCmax;
                    bestSeq = startSeq;
                }
            }
            else
            {
                std::swap(startSeq[pos1], startSeq[pos2]);
            }

            temperature *= cooling;
        }

        std::cout << "Final best makespan: " << bestCmax << std::endl;

        Schedule(bestSeq).emitFinalSlots(instance);
        return Schedule(bestSeq);
    }
};
