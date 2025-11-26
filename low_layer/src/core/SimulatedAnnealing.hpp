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
        int n = instance.getJobs();

        std::vector<int> currentSeq;
        if (initialSolution.getJobSequence().empty())
        {

            currentSeq.resize(n);
            for (int i = 0; i < n; i++)
            {
                currentSeq[i] = i;
            }
            std::shuffle(currentSeq.begin(), currentSeq.end(), rng);
        }
        else
        {
            currentSeq = initialSolution.getJobSequence();
        }

        return runSAFromSeq(currentSeq, maxIterations, initialTemperature, coolingFactor, "SIMULATED_ANNEALING");
    }

    Schedule solveAdaptive(const Schedule &initialSolution = Schedule())
    {
        int n = instance.getJobs();

        std::vector<int> currentSeq;
        if (initialSolution.getJobSequence().empty())
        {
            currentSeq.resize(n);
            for (int i = 0; i < n; i++)
            {
                currentSeq[i] = i;
            }
            std::shuffle(currentSeq.begin(), currentSeq.end(), rng);
        }
        else
        {
            currentSeq = initialSolution.getJobSequence();
        }

        std::vector<int> statsSeq = currentSeq;
        double statsCurrent = instance.computeMakespan(statsSeq);
        double statsBestCmax = statsCurrent;

        std::uniform_int_distribution<int> jobDist(0, n - 1);

        int sumSteps = 0;
        int statsIterations = maxIterations / 10;
        if (statsIterations <= 0)
            statsIterations = 1;

        for (int i = 0; i < statsIterations; i++)
        {
            int pos1 = jobDist(rng);
            int pos2 = jobDist(rng);

            std::swap(statsSeq[pos1], statsSeq[pos2]);
            double newCmax = instance.computeMakespan(statsSeq);
            int difference = std::abs(static_cast<int>(newCmax - statsCurrent));
            sumSteps += difference;

            statsCurrent = newCmax;
            if (newCmax < statsBestCmax)
            {
                statsBestCmax = newCmax;
            }
        }

        int averageStep = sumSteps / statsIterations / 15;
        if (averageStep == 0)
            averageStep = 1;

        double temperature = -averageStep / std::log(0.8);
        double coolingFactorAdaptive = std::pow(std::log(0.8) / std::log(0.001), 1.0 / (maxIterations * 0.95));

        std::cout << "Phase 2: Optimization with adaptive temperature: " << temperature << std::endl;

        Schedule phase2 = runSAFromSeq(currentSeq, maxIterations, temperature, coolingFactorAdaptive, "ADAPTIVE_SA");
        return phase2;
    }

private:
    void generateVisualization(const std::vector<int> &sequence)
    {
        Schedule(sequence).emitFinalSlots(instance);
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
            if (iteration % 5000 == 0 && iteration > 0)
            {
                std::cout << "Iteration " << iteration << ", current best: " << bestCmax << std::endl;
            }
        }

        std::cout << "Final best makespan: " << bestCmax << std::endl;

        generateVisualization(bestSeq);

        return Schedule(bestSeq);
    }
};
