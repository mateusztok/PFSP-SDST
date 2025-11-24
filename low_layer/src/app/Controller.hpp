#pragma once
#include <string>
#include "../core/Instance.hpp"
#include "../core/Schedule.hpp"
#include "../core/SimulatedAnnealing.hpp"
#include "../core/NEHWithProgress.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <vector>
#include <cctype>

class Controller
{
private:
    Instance instance;
    Schedule schedule;

    std::vector<std::string> tokensFromArgs;

public:
    Controller(const std::string &instancePath, const std::vector<std::string> &algorithmArgs)
        : instance(instancePath), tokensFromArgs(algorithmArgs)
    {
    }

    void execute()
    {
        instance.loadFromFile();

        auto hasToken = [&](const std::string &t) -> bool
        {
            return std::find(tokensFromArgs.begin(), tokensFromArgs.end(), t) != tokensFromArgs.end();
        };

        Schedule currentBest;

        if (hasToken("neh"))
        {
            std::cout << "\n=== Phase: NEH ===" << std::endl;
            currentBest = runNEHWithProgress();
            std::cout << "\n=== Phase: NEH end===" << std::endl;
        }

        if (hasToken("simulated_annealing"))
        {
            std::cout << "\n=== Phase: Simulated Annealing ===" << std::endl;
            if (currentBest.getJobSequence().empty())
            {
                std::cout << "No initial solution from NEH, starting with random." << std::endl;
            }
            else
            {
                std::cout << "Starting from NEH solution with makespan: "
                          << instance.computeMakespan(currentBest.getJobSequence()) << std::endl;
            }
            currentBest = runSimulatedAnnealing(currentBest);
        }

        if (hasToken("adaptive_sa"))
        {
            std::cout << "\n=== Phase: Adaptive Simulated Annealing ===" << std::endl;
            if (currentBest.getJobSequence().empty())
            {
                std::cout << "No initial solution, starting with random." << std::endl;
            }
            else
            {
                std::cout << "Starting from previous solution with makespan: "
                          << instance.computeMakespan(currentBest.getJobSequence()) << std::endl;
            }
            currentBest = runAdaptiveSimulatedAnnealing(currentBest);
        }

        if (!hasToken("neh") && !hasToken("simulated_annealing") && !hasToken("adaptive_sa"))
        {
            std::cout << "No algorithm matched. Provided: ";
            for (size_t i = 0; i < tokensFromArgs.size(); ++i)
            {
                if (i)
                    std::cout << ",";
                std::cout << tokensFromArgs[i];
            }
            std::cout << std::endl;
            std::cout << "Available algorithms:" << std::endl;
            std::cout << "  - neh" << std::endl;
            std::cout << "  - simulated_annealing" << std::endl;
            std::cout << "  - adaptive_sa" << std::endl;
            std::cout << "  - neh+simulated_annealing" << std::endl;
            std::cout << "  - neh+adaptive_sa" << std::endl;
            std::cout << "  - neh+simulated_annealing+adaptive_sa" << std::endl;
        }

        if (!currentBest.getJobSequence().empty())
        {
            std::cout << "\n=== FINAL RESULT ===" << std::endl;
            std::cout << "Final best sequence: ";
            currentBest.print();
            double finalMakespan = instance.computeMakespan(currentBest.getJobSequence());
            std::cout << "Final best makespan: " << finalMakespan << std::endl;
        }
    }

private:
    Schedule runSimulatedAnnealing(const Schedule &initialSolution = Schedule())
    {
        std::cout << "Running Simulated Annealing..." << std::endl;

        auto start = std::chrono::high_resolution_clock::now();

        SimulatedAnnealing simAnneal(instance, 1);
        simAnneal.setParameters(50000, 100.0, 0.9975);

        Schedule result = simAnneal.solve(initialSolution);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "Simulated Annealing best sequence: ";
        result.print();

        double makespan = instance.computeMakespan(result.getJobSequence());
        std::cout << "Simulated Annealing final makespan: " << makespan << std::endl;
        std::cout << "Simulated Annealing execution time: " << duration.count() << " ms" << std::endl;

        return result;
    }

    Schedule runAdaptiveSimulatedAnnealing(const Schedule &initialSolution = Schedule())
    {
        std::cout << "Running Adaptive Simulated Annealing..." << std::endl;

        auto start = std::chrono::high_resolution_clock::now();

        SimulatedAnnealing simAnneal(instance, 1);
        simAnneal.setParameters(50000, 100.0, 0.9975);

        Schedule result = simAnneal.solveAdaptive(initialSolution);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "Adaptive Simulated Annealing best sequence: ";
        result.print();

        double makespan = instance.computeMakespan(result.getJobSequence());
        std::cout << "Adaptive Simulated Annealing final makespan: " << makespan << std::endl;
        std::cout << "Adaptive Simulated Annealing execution time: " << duration.count() << " ms" << std::endl;

        return result;
    }

    Schedule runNEHWithProgress()
    {
        auto start = std::chrono::high_resolution_clock::now();
        NEHWithProgress neh(instance);
        
        Schedule result = neh.solve();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        double nehMakespan = instance.computeMakespan(result.getJobSequence());
        std::cout << "NEH final makespan: " << nehMakespan << ", execution time: " << duration.count() << " ms" << std::endl;

        return result;
    }
};
