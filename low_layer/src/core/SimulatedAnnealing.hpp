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

        std::vector<int> bestSeq = currentSeq;
        double currentCmax = instance.computeMakespan(currentSeq);
        double bestCmax = currentCmax;

        double temperature = initialTemperature;
        std::uniform_int_distribution<int> jobDist(0, n - 1);
        std::uniform_real_distribution<double> probDist(0.0, 1.0);

        std::cout << "Starting Simulated Annealing..." << std::endl;
        std::cout << "Initial makespan: " << currentCmax << std::endl;

        for (int iteration = 0; iteration < maxIterations; iteration++)
        {

            int pos1 = jobDist(rng);
            int pos2 = jobDist(rng);

            std::swap(currentSeq[pos1], currentSeq[pos2]);

            double newCmax = instance.computeMakespan(currentSeq);
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
                    bestSeq = currentSeq;

                    if (iteration % 5000 == 0)
                    {
                        std::cout << "Iteration " << iteration << ", new best: " << bestCmax << std::endl;
                    }
                }
            }
            else
            {

                std::swap(currentSeq[pos1], currentSeq[pos2]);
            }

            temperature *= coolingFactor;
        }

        std::cout << "Final best makespan: " << bestCmax << std::endl;

        generateVisualization(bestSeq, "SIMULATED_ANNEALING");

        return Schedule(bestSeq);
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

        std::vector<int> bestSeq = currentSeq;
        double currentCmax = instance.computeMakespan(currentSeq);
        double bestCmax = currentCmax;

        std::uniform_int_distribution<int> jobDist(0, n - 1);
        std::uniform_real_distribution<double> probDist(0.0, 1.0);

        std::cout << "Adaptive Simulated Annealing - Phase 1: Statistics..." << std::endl;
        int sumSteps = 0;
        int statsIterations = maxIterations / 10;

        for (int i = 0; i < statsIterations; i++)
        {
            int pos1 = jobDist(rng);
            int pos2 = jobDist(rng);

            std::swap(currentSeq[pos1], currentSeq[pos2]);
            double newCmax = instance.computeMakespan(currentSeq);
            int difference = std::abs(static_cast<int>(newCmax - currentCmax));
            sumSteps += difference;

            currentCmax = newCmax;
            if (newCmax < bestCmax)
            {
                bestCmax = newCmax;
                bestSeq = currentSeq;
            }
        }

        int averageStep = sumSteps / statsIterations / 15;
        if (averageStep == 0)
            averageStep = 1;

        double temperature = -averageStep / std::log(0.8);
        double coolingFactorAdaptive = std::pow(std::log(0.8) / std::log(0.001),
                                                1.0 / (maxIterations * 0.95));

        std::cout << "Phase 2: Optimization with adaptive temperature: " << temperature << std::endl;
        bestCmax = 1e9;

        for (int iteration = 0; iteration < maxIterations; iteration++)
        {
            int pos1 = jobDist(rng);
            int pos2 = jobDist(rng);

            std::swap(currentSeq[pos1], currentSeq[pos2]);
            double newCmax = instance.computeMakespan(currentSeq);
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
                    bestSeq = currentSeq;

                    if (iteration % 5000 == 0)
                    {
                        std::cout << "Iteration " << iteration << ", new best: " << bestCmax << std::endl;
                    }
                }
            }
            else
            {
                std::swap(currentSeq[pos1], currentSeq[pos2]);
            }

            temperature *= coolingFactorAdaptive;
            if (iteration > 0.95 * maxIterations)
            {
                coolingFactorAdaptive *= 0.999;
            }
        }

        std::cout << "Final adaptive best makespan: " << bestCmax << std::endl;

        generateVisualization(bestSeq, "ADAPTIVE_SA");

        return Schedule(bestSeq);
    }

private:
    void generateVisualization(const std::vector<int> &sequence, const std::string & /*prefix*/)
    {
        Schedule(sequence).emitFinalSlots(instance);
    }
};
