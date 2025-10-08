#pragma once
#include <string>
#include "../core/Instance.hpp"
#include "../core/Schedule.hpp"
#include "../core/SimulatedAnnealing.hpp"
#include "../core/QuickNEH.hpp"
#include "../core/NEHWithProgress.hpp"
#include <iostream>
#include <chrono>

class Controller
{
private:
    Instance instance;
    Schedule schedule;
    std::string algorithmName;

public:
    Controller(const std::string &instancePath, const std::string &algorithmName)
        : instance(instancePath), algorithmName(algorithmName) {}

    void execute()
    {
        instance.loadFromFile();

        std::cout << "Algorithm name: '" << algorithmName << "'" << std::endl;

        // Sprawdź czy algorytm zawiera kombinację (np. "quick_neh+sa")
        bool useNEHProgress = (algorithmName.find("neh_progress") != std::string::npos);
        bool useQuickNEH = (algorithmName.find("quick_neh") != std::string::npos ||
                            (algorithmName.find("neh") != std::string::npos && !useNEHProgress));
        bool useSA = (algorithmName.find("simulated_annealing") != std::string::npos ||
                      algorithmName.find("sa") != std::string::npos);
        bool useASA = (algorithmName.find("adaptive_sa") != std::string::npos ||
                       algorithmName.find("asa") != std::string::npos);

        Schedule currentBest; // Będzie przekazywany między algorytmami

        // 1. NEH with Progress - nowa ulepszona wersja
        if (useNEHProgress)
        {
            std::cout << "\n=== Phase 1: NEH with Progress ===" << std::endl;
            currentBest = runNEHWithProgress();
        }
        // 1b. QuickNEH - stara wersja (jeśli nie ma progress)
        else if (useQuickNEH)
        {
            std::cout << "\n=== Phase 1: Quick NEH ===" << std::endl;
            currentBest = runQuickNEH();
        }

        // 2. Simulated Annealing - używa wyniku z QuickNEH jako punkt startowy
        if (useSA)
        {
            std::cout << "\n=== Phase 2: Simulated Annealing ===" << std::endl;
            if (currentBest.getJobSequence().empty())
            {
                std::cout << "No initial solution from QuickNEH, starting with random." << std::endl;
            }
            else
            {
                std::cout << "Starting from QuickNEH solution with makespan: "
                          << instance.computeMakespan(currentBest.getJobSequence()) << std::endl;
            }
            currentBest = runSimulatedAnnealing(currentBest);
        }

        // 3. Adaptive SA - używa wyniku z poprzednich algorytmów
        if (useASA)
        {
            std::cout << "\n=== Phase 3: Adaptive Simulated Annealing ===" << std::endl;
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

        // Jeśli żaden algorytm nie został dopasowany
        if (!useQuickNEH && !useNEHProgress && !useSA && !useASA)
        {
            std::cout << "No algorithm matched, algorithm name: '" << algorithmName << "'" << std::endl;
            std::cout << "Available algorithms:" << std::endl;
            std::cout << "  - quick_neh (or neh)" << std::endl;
            std::cout << "  - neh_progress" << std::endl;
            std::cout << "  - simulated_annealing (or sa)" << std::endl;
            std::cout << "  - adaptive_sa (or asa)" << std::endl;
            std::cout << "  - neh_progress+sa" << std::endl;
            std::cout << "  - neh_progress+asa" << std::endl;
            std::cout << "  - quick_neh+sa+asa" << std::endl;
        }

        // Podsumowanie końcowe
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

        SimulatedAnnealing sa(instance, 42);    // seed = 42 dla powtarzalności
        sa.setParameters(50000, 100.0, 0.9975); // iteracje, temp_początkowa, czynnik_chłodzenia

        Schedule result = sa.solve(initialSolution);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "SA best sequence: ";
        result.print();

        double makespan = instance.computeMakespan(result.getJobSequence());
        std::cout << "SA final makespan: " << makespan << std::endl;
        std::cout << "SA execution time: " << duration.count() << " ms" << std::endl;

        return result;
    }

    Schedule runAdaptiveSimulatedAnnealing(const Schedule &initialSolution = Schedule())
    {
        std::cout << "Running Adaptive Simulated Annealing..." << std::endl;

        auto start = std::chrono::high_resolution_clock::now();

        SimulatedAnnealing sa(instance, 42);    // seed = 42 dla powtarzalności
        sa.setParameters(50000, 100.0, 0.9975);
        sa.setParameters(50000, 100.0, 0.9975);

        Schedule result = sa.solveAdaptive(initialSolution);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "ASA best sequence: ";
        result.print();

        double makespan = instance.computeMakespan(result.getJobSequence());
        std::cout << "ASA final makespan: " << makespan << std::endl;
        std::cout << "ASA execution time: " << duration.count() << " ms" << std::endl;

        return result;
    }

    Schedule runQuickNEH()
    {
        std::cout << "Running Quick NEH Algorithm..." << std::endl;

        auto start = std::chrono::high_resolution_clock::now();

        QuickNEH neh(instance);
        Schedule result = neh.solve();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "NEH best sequence: ";
        result.print();

        double makespan = instance.computeMakespan(result.getJobSequence());
        std::cout << "NEH final makespan: " << makespan << std::endl;
        std::cout << "NEH execution time: " << duration.count() << " ms" << std::endl;

        return result;
    }

    Schedule runNEHWithProgress()
    {
        std::cout << "Running NEH with Progress Logging..." << std::endl;

        auto start = std::chrono::high_resolution_clock::now();

        NEHWithProgress neh(instance);
        Schedule result = neh.solve(true); // verbose = true

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "NEH Progress final makespan: " << instance.computeMakespan(result.getJobSequence()) << std::endl;
        std::cout << "NEH Progress execution time: " << duration.count() << " ms" << std::endl;

        return result;
    }
};
