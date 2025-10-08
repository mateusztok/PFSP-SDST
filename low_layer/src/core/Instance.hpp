#pragma once
#include <vector>
#include <string>
#include "Schedule.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
class Instance
{
private:
    int jobs;
    int machines;
    std::vector<std::vector<int>> proc_time; // czas wykonywania zadań
    std::vector<std::vector<std::vector<int>>> setup_time;
    const std::string filePath; // czasy przezbrojeń: setup_time[machine][prevJob][currJob]

public:
    Instance(const std::string &filename) : filePath(filename) {}
    // Wczytywanie danych z pliku
    inline void loadFromFile()
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            std::cerr << "Error: cannot open file " << filePath << std::endl;
            return;
        }

        std::string line;
        // --- Wczytanie liczby zadań i maszyn ---
        while (std::getline(file, line))
        {
            if (line.find("jobs") != std::string::npos)
            {
                std::stringstream ss(line);
                std::string dummy;
                ss >> dummy >> dummy >> jobs; // jobs = N
            }
            else if (line.find("machines") != std::string::npos)
            {
                std::stringstream ss(line);
                std::string dummy;
                ss >> dummy >> dummy >> machines; // machines = M
            }
            else if (line.find("proc_time") != std::string::npos)
            {
                break;
            }
        }

        // --- Wczytanie czasów przetwarzania ---
        proc_time.assign(jobs, std::vector<int>(machines, 0));
        for (int j = 0; j < jobs; ++j)
        {
            for (int m = 0; m < machines; ++m)
            {
                file >> proc_time[j][m];
            }
        }

        // --- Wczytanie czasów przezbrojeń ---
        setup_time.assign(machines, std::vector<std::vector<int>>(jobs, std::vector<int>(jobs, 0)));

        for (int m = 0; m < machines; ++m)
        {
            // szukamy linii "# machine X"
            while (std::getline(file, line))
            {
                if (line.find("# machine") != std::string::npos)
                    break;
            }

            for (int i = 0; i < jobs; ++i)
            {
                for (int j = 0; j < jobs; ++j)
                {
                    file >> setup_time[m][i][j];
                }
            }
        }

        file.close();

        // --- Podstawowe informacje o wczytanej instancji ---
        std::cout << "Instance loaded: " << jobs << " jobs, " << machines << " machines" << std::endl;
    }

    // Gettery
    int getJobs() const { return jobs; }
    int getMachines() const { return machines; }

    // Getter czasów przetwarzania dla zadania i na maszynie
    int getProcTime(int job, int machine) const
    {
        return proc_time.at(job).at(machine);
    }

    // Getter czasów przezbrojeń: dla danej maszyny, poprzedniego zadania i bieżącego zadania
    int getSetupTime(int machine, int prevJob, int currJob) const
    {
        return setup_time.at(machine).at(prevJob).at(currJob);
    }

    // Alternatywnie można udostępnić całe tablice jako const reference
    const std::vector<std::vector<int>> &getProcTimes() const { return proc_time; }
    const std::vector<std::vector<std::vector<int>>> &getSetupTimes() const { return setup_time; }

    inline double computeMakespan(const std::vector<int> &seq) const
    {
        if (seq.empty())
            return 0.0;

        int n = seq.size();
        int m = machines;

        // Tablice do przechowywania czasów zakończenia zadań na maszynach
        std::vector<std::vector<double>> C(n, std::vector<double>(m, 0.0));

        for (int pos = 0; pos < n; ++pos)
        {
            int currentJob = seq[pos];

            for (int machine = 0; machine < m; ++machine)
            {
                double startTime = 0.0;

                // Warunek 1: Maszyna musi być dostępna (poprzednie zadanie skończone + setup)
                if (pos > 0)
                {
                    int prevJob = seq[pos - 1];
                    double setupTime = setup_time[machine][prevJob][currentJob];
                    startTime = C[pos - 1][machine] + setupTime;
                }

                // Warunek 2: Poprzednia operacja tego zadania musi być skończona
                if (machine > 0)
                {
                    startTime = std::max(startTime, C[pos][machine - 1]);
                }

                // Czas zakończenia = czas rozpoczęcia + czas przetwarzania
                double procTime = proc_time[currentJob][machine];
                C[pos][machine] = startTime + procTime;
            }
        }

        // Makespan to czas zakończenia ostatniego zadania na ostatniej maszynie
        return C[n - 1][m - 1];
    }
};
