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

        // Lambda do sprawdzania obecności flagi
        auto hasToken = [&](const std::string &t) -> bool
        {
            return std::find(tokensFromArgs.begin(), tokensFromArgs.end(), t) != tokensFromArgs.end();
        };

        Schedule currentBest;

        // Faza NEH
        if (hasToken("neh"))
        {
            std::cout << "\n=== Phase: NEH ===" << std::endl;
            currentBest = runNEHWithProgress();
            std::cout << "\n=== Phase: NEH end===" << std::endl;
        }

        // Faza Simulated Annealing
        auto saIt = std::find(tokensFromArgs.begin(), tokensFromArgs.end(), "simulated_annealing");
        if (saIt != tokensFromArgs.end())
        {
            std::cout << "\n=== Phase: Simulated Annealing ===" << std::endl;
            
            // Domyślne parametry
            int iters = 50000;
            double temp = 100.0;
            double cooling = 0.9975;

            // Parsowanie parametrów z argumentów (jeśli istnieją po słowie kluczowym)
            try {
                auto next = std::next(saIt);
                if (next != tokensFromArgs.end()) {
                    iters = std::stoi(*next); // Pierwszy parametr: Iteracje
                    if (++next != tokensFromArgs.end()) {
                        temp = std::stod(*next); // Drugi parametr: Temperatura
                        if (++next != tokensFromArgs.end()) {
                            cooling = std::stod(*next); // Trzeci parametr: Chłodzenie
                        }
                    }
                }
            } catch (const std::exception& e) {
                std::cout << "Warning: Could not parse SA parameters, using defaults. Error: " << e.what() << std::endl;
            }

            std::cout << "Parameters: Iterations=" << iters << ", Temp=" << temp << ", Cooling=" << cooling << std::endl;

            if (currentBest.getJobSequence().empty()) {
                std::cout << "No initial solution from NEH, starting with random." << std::endl;
            } else {
                std::cout << "Starting from NEH solution with makespan: "
                          << instance.computeMakespan(currentBest.getJobSequence()) << std::endl;
            }

            // Uruchomienie SA z pobranymi parametrami
            currentBest = runSimulatedAnnealing(currentBest, iters, temp, cooling);
        }

        // Wyświetlenie wyniku końcowego
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
    // Zmodyfikowana metoda przyjmująca parametry
    Schedule runSimulatedAnnealing(const Schedule &initialSolution, int iters, double temp, double cooling)
    {
        std::cout << "Running Simulated Annealing..." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();

        SimulatedAnnealing simAnneal(instance, 1);
        simAnneal.setParameters(iters, temp, cooling); // Przekazanie parametrów do algorytmu

        Schedule result = simAnneal.solve(initialSolution);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        double makespan = instance.computeMakespan(result.getJobSequence());
        std::cout << "Simulated Annealing final makespan: " << makespan << std::endl;
        std::cout << "Simulated Annealing execution time: " << duration.count() << " ms" << std::endl;

        return result;
    }

    Schedule runNEHWithProgress()
    {
        auto start = std::chrono::high_resolution_clock::now();
        NEHWithProgress neh(instance);
        Schedule result = neh.solve();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return result;
    }
};