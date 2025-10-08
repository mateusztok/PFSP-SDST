#pragma once
#include <vector>
#include <algorithm>
#include <iostream>
#include <climits>
#include "Instance.hpp"
#include "Schedule.hpp"

class QuickNEH
{
private:
    const Instance &instance;

    struct Times
    {
        int execution_time;
        int time_from_begin;
        int time_from_end;
    };

public:
    QuickNEH(const Instance &inst) : instance(inst) {}

    // Główna metoda algorytmu Quick NEH
    Schedule solve()
    {
        int n = instance.getJobs();
        int m = instance.getMachines();

        std::cout << "Starting Quick NEH algorithm..." << std::endl;

        // Stworzenie kopii zadań z obliczeniem całkowitego czasu
        std::vector<std::pair<int, int>> jobTotalTimes; // (job_id, total_time)

        for (int i = 0; i < n; i++)
        {
            int totalTime = 0;
            for (int j = 0; j < m; j++)
            {
                totalTime += instance.getProcTime(i, j);
            }
            jobTotalTimes.push_back({i, totalTime});
        }

        // Sortowanie zadań według całkowitego czasu (malejąco)
        std::sort(jobTotalTimes.begin(), jobTotalTimes.end(),
                  [](const std::pair<int, int> &a, const std::pair<int, int> &b)
                  {
                      if (a.second != b.second)
                      {
                          return a.second > b.second; // malejąco według czasu
                      }
                      return a.first < b.first; // rosnąco według ID w przypadku równości
                  });

        std::cout << "Jobs sorted by total processing time (descending):" << std::endl;
        for (const auto &job : jobTotalTimes)
        {
            std::cout << "Job " << job.first << " (total time: " << job.second << ") ";
        }
        std::cout << std::endl;

        // Tworzenie sekwencji zadań
        std::vector<int> jobSequence;
        for (const auto &job : jobTotalTimes)
        {
            jobSequence.push_back(job.first);
        }

        // Algorytm Quick NEH - budowanie sekwencji krok po kroku
        std::vector<int> currentSequence;

        for (int i = 0; i < n; i++)
        {
            int newJob = jobSequence[i];

            if (i == 0)
            {
                // Pierwsze zadanie - po prostu dodaj
                currentSequence.push_back(newJob);
            }
            else
            {
                // Znajdź najlepszą pozycję dla nowego zadania
                int bestPosition = findBestPosition(currentSequence, newJob);
                currentSequence.insert(currentSequence.begin() + bestPosition, newJob);
            }

            std::cout << "Step " << (i + 1) << ", added job " << newJob
                      << ", current makespan: " << computeMakespan(currentSequence) << std::endl;
        }

        double finalMakespan = computeMakespan(currentSequence);
        std::cout << "Quick NEH final makespan: " << finalMakespan << std::endl;

        // Wizualizacja końcowego wyniku
        generateVisualization(currentSequence, "QUICK_NEH");

        // Output dla GUI
        std::cout << "QUICK_NEH_RESULT;perm=";
        for (size_t i = 0; i < currentSequence.size(); ++i)
        {
            std::cout << (i ? "," : "") << currentSequence[i];
        }
        std::cout << ";cmax=" << finalMakespan << std::endl;
        std::cout << "END" << std::endl;

        return Schedule(currentSequence);
    }

private:
    // Znajdź najlepszą pozycję dla wstawienia nowego zadania
    int findBestPosition(const std::vector<int> &currentSeq, int newJob)
    {
        int bestPosition = 0;
        double bestMakespan = 1e9;

        // Przetestuj wszystkie możliwe pozycje
        for (int pos = 0; pos <= (int)currentSeq.size(); pos++)
        {
            std::vector<int> testSeq = currentSeq;
            testSeq.insert(testSeq.begin() + pos, newJob);

            double makespan = computeMakespan(testSeq);
            if (makespan < bestMakespan)
            {
                bestMakespan = makespan;
                bestPosition = pos;
            }
        }

        return bestPosition;
    }

    // Oblicz makespan dla sekwencji (bez setup times - klasyczny flow shop)
    double computeMakespan(const std::vector<int> &sequence)
    {
        if (sequence.empty())
            return 0.0;

        int n = sequence.size();
        int m = instance.getMachines();

        // Tablica czasów zakończenia [zadanie][maszyna]
        std::vector<std::vector<double>> C(n, std::vector<double>(m, 0.0));

        for (int i = 0; i < n; i++)
        {
            int job = sequence[i];

            for (int machine = 0; machine < m; machine++)
            {
                double startTime = 0.0;

                // Warunek 1: Czekaj na poprzednie zadanie na tej samej maszynie + setup
                if (i > 0)
                {
                    int prevJob = sequence[i - 1];
                    double setupTime = instance.getSetupTime(machine, prevJob, job);
                    startTime = C[i - 1][machine] + setupTime;
                }

                // Warunek 2: Czekaj na poprzednią maszynę tego samego zadania
                if (machine > 0)
                {
                    startTime = std::max(startTime, C[i][machine - 1]);
                }

                // Czas zakończenia = start + czas przetwarzania
                C[i][machine] = startTime + instance.getProcTime(job, machine);
            }
        }

        return C[n - 1][m - 1];
    }

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
