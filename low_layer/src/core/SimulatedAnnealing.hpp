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

    // Parametry algorytmu
    int maxIterations;
    double initialTemperature;
    double coolingFactor;

public:
    SimulatedAnnealing(const Instance &inst, int seed = 0)
        : instance(inst), rng(seed)
    {
        // Domyślne parametry
        maxIterations = 50000;
        initialTemperature = 100.0;
        coolingFactor = 0.9975;
    }

    // Ustawienie parametrów algorytmu
    void setParameters(int iterations, double temp, double cooling)
    {
        maxIterations = iterations;
        initialTemperature = temp;
        coolingFactor = cooling;
    }

    // Główna metoda algorytmu symulowanego wyżarzania
    Schedule solve(const Schedule &initialSolution = Schedule())
    {
        int n = instance.getJobs();

        // Inicjalizacja rozwiązania
        std::vector<int> currentSeq;
        if (initialSolution.getJobSequence().empty())
        {
            // Jeśli nie ma rozwiązania początkowego, stwórz losowe
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

        // Najlepsze znalezione rozwiązanie
        std::vector<int> bestSeq = currentSeq;
        double currentCmax = instance.computeMakespan(currentSeq);
        double bestCmax = currentCmax;

        // Parametry algorytmu
        double temperature = initialTemperature;
        std::uniform_int_distribution<int> jobDist(0, n - 1);
        std::uniform_real_distribution<double> probDist(0.0, 1.0);

        std::cout << "Starting Simulated Annealing..." << std::endl;
        std::cout << "Initial makespan: " << currentCmax << std::endl;

        for (int iteration = 0; iteration < maxIterations; iteration++)
        {
            // Generuj sąsiada przez swap dwóch losowych zadań
            int pos1 = jobDist(rng);
            int pos2 = jobDist(rng);

            // Wykonaj swap
            std::swap(currentSeq[pos1], currentSeq[pos2]);

            // Oblicz nowy makespan
            double newCmax = instance.computeMakespan(currentSeq);
            double delta = newCmax - currentCmax;

            // Sprawdź czy zaakceptować nowe rozwiązanie
            bool accept = false;
            if (delta < 0)
            {
                // Lepsze rozwiązanie - zawsze akceptuj
                accept = true;
            }
            else
            {
                // Gorsze rozwiązanie - akceptuj z prawdopodobieństwem
                double probability = std::exp(-delta / temperature);
                accept = (probDist(rng) < probability);
            }

            if (accept)
            {
                currentCmax = newCmax;

                // Sprawdź czy to najlepsze dotychczas znalezione
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
                // Cofnij swap
                std::swap(currentSeq[pos1], currentSeq[pos2]);
            }

            // Ochładzanie
            temperature *= coolingFactor;
        }

        std::cout << "Final best makespan: " << bestCmax << std::endl;

        // Wizualizacja końcowego wyniku
        generateVisualization(bestSeq, "SA");

        // Output dla GUI
        std::cout << "SA_RESULT;perm=";
        for (size_t i = 0; i < bestSeq.size(); ++i)
        {
            std::cout << (i ? "," : "") << bestSeq[i];
        }
        std::cout << ";cmax=" << bestCmax << std::endl;
        std::cout << "END" << std::endl;

        return Schedule(bestSeq);
    }

    // Adaptacyjne symulowane wyżarzanie (z Twojego kodu)
    Schedule solveAdaptive(const Schedule &initialSolution = Schedule())
    {
        int n = instance.getJobs();

        // Inicjalizacja rozwiązania
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

        // Faza 1: Zbieranie statystyk do obliczenia temperatury początkowej
        std::cout << "Adaptive Simulated Annealing - Phase 1: Statistics..." << std::endl;
        int sumSteps = 0;
        int statsIterations = maxIterations / 10; // 10% iteracji na statystyki

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

        // Oblicz temperaturę początkową
        int averageStep = sumSteps / statsIterations / 15;
        if (averageStep == 0)
            averageStep = 1;

        double temperature = -averageStep / std::log(0.8);
        double coolingFactorAdaptive = std::pow(std::log(0.8) / std::log(0.001),
                                                1.0 / (maxIterations * 0.95));

        std::cout << "Phase 2: Optimization with adaptive temperature: " << temperature << std::endl;
        bestCmax = 1e9; // Reset dla właściwej optymalizacji

        // Faza 2: Właściwa optymalizacja
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

            // Adaptacyjne ochładzanie
            temperature *= coolingFactorAdaptive;
            if (iteration > 0.95 * maxIterations)
            {
                coolingFactorAdaptive *= 0.999;
            }
        }

        std::cout << "Final adaptive best makespan: " << bestCmax << std::endl;

        // Wizualizacja końcowego wyniku
        generateVisualization(bestSeq, "ASA");

        // Output dla GUI
        std::cout << "ASA_RESULT;perm=";
        for (size_t i = 0; i < bestSeq.size(); ++i)
        {
            std::cout << (i ? "," : "") << bestSeq[i];
        }
        std::cout << ";cmax=" << bestCmax << std::endl;
        std::cout << "END" << std::endl;

        return Schedule(bestSeq);
    }

private:
    // Funkcja do generowania wizualizacji (slotów) dla GUI
    void generateVisualization(const std::vector<int> &sequence, const std::string &prefix)
    {
        int m = instance.getMachines();

        if (sequence.empty())
            return;

        // Symuluj harmonogram dla wizualizacji
        std::vector<std::vector<long long>> machineTime(m, std::vector<long long>(1, 0));

        struct OpRec
        {
            int job;
            long long start;
            long long end;
            int setup_used;
        };
        std::vector<std::vector<OpRec>> schedule(m);

        for (int job : sequence)
        {
            for (int mach = 0; mach < m; ++mach)
            {
                long long procTime = instance.getProcTime(job, mach);
                long long setupTime = 0;

                if (!schedule[mach].empty())
                {
                    int prevJob = schedule[mach].back().job;
                    setupTime = instance.getSetupTime(mach, prevJob, job);
                }

                long long startTime = machineTime[mach][0] + setupTime;
                if (mach > 0)
                {
                    // Czekaj na zakończenie na poprzedniej maszynie
                    for (const auto &op : schedule[mach - 1])
                    {
                        if (op.job == job)
                        {
                            startTime = std::max(startTime, op.end);
                            break;
                        }
                    }
                }

                long long endTime = startTime + procTime;
                machineTime[mach][0] = endTime;
                schedule[mach].push_back({job, startTime, endTime, (int)setupTime});
            }
        }

        // Wypisz sloty dla GUI
        for (int m = 0; m < (int)schedule.size(); ++m)
        {
            int prevJob = -1;
            for (const auto &op : schedule[m])
            {
                std::cout << "SLOT;iter=FINAL"
                          << ";machine=" << m
                          << ";prev=" << (prevJob < 0 ? -1 : prevJob)
                          << ";job=" << op.job
                          << ";setup=" << op.setup_used
                          << ";start=" << op.start
                          << ";end=" << op.end << std::endl;
                prevJob = op.job;
            }
        }
        std::cout << "FRAME_END;iter=FINAL" << std::endl;
    }
};
