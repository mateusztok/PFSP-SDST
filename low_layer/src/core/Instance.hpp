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
    int jobs = 0;
    int machines = 0;
    std::vector<std::vector<int>> proc_time;
    std::vector<std::vector<std::vector<int>>> setup_time;
    const std::string filePath;

public:
    Instance(const std::string &filename) : filePath(filename) {}
    inline void loadFromFile()
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            std::cerr << "Error: cannot open file " << filePath << std::endl;
            return;
        }

        std::string line;
        while (std::getline(file, line))
        {
            if (line.find("jobs") != std::string::npos)
            {
                std::stringstream ss(line);
                std::string dummy;
                ss >> dummy >> dummy >> jobs;
            }
            else if (line.find("machines") != std::string::npos)
            {
                std::stringstream ss(line);
                std::string dummy;
                ss >> dummy >> dummy >> machines;
            }
            else if (line.find("proc_time") != std::string::npos)
            {
                break;
            }
        }

        proc_time.assign(jobs, std::vector<int>(machines, 0));
        for (int j = 0; j < jobs; ++j)
        {
            std::string l;
            do
            {
                if (!std::getline(file, l))
                {
                    std::cerr << "Error: unexpected EOF while reading proc_time (job=" << j << ") in " << filePath << std::endl;
                    return;
                }
            } while (l.find_first_not_of(" \t\r\n") == std::string::npos);

            std::stringstream ss(l);
            for (int m = 0; m < machines; ++m)
            {
                int val = 0;
                if (!(ss >> val))
                {
                    std::cerr << "Error: failed to parse proc_time at job=" << j << " machine=" << m
                              << " from line: '" << l << "' in " << filePath << std::endl;
                    return;
                }
                proc_time[j][m] = val;
            }
        }

        long long procSum = 0;
        for (int j = 0; j < jobs; ++j)
            for (int m = 0; m < machines; ++m)
                procSum += proc_time[j][m];

        if (procSum == 0)
        {
            std::cerr << "Warning: all proc_time values are zero after parsing " << filePath
                      << ". Check input file format or parsing logic." << std::endl;

            std::cerr << "proc_time sample:\n";
            for (int j = 0; j < std::min(jobs, 3); ++j)
            {
                for (int m = 0; m < std::min(machines, 3); ++m)
                    std::cerr << proc_time[j][m] << " ";
                std::cerr << "\n";
            }
        }

        setup_time.assign(machines, std::vector<std::vector<int>>(jobs, std::vector<int>(jobs, 0)));

        for (int m = 0; m < machines; ++m)
        {

            while (std::getline(file, line))
            {
                if (line.find("# machine") != std::string::npos)
                    break;
            }

            for (int i = 0; i < jobs; ++i)
            {
                std::string l;
                do
                {
                    if (!std::getline(file, l))
                    {
                        std::cerr << "Error: unexpected EOF while reading setup_time (machine=" << m << " row=" << i << ") in " << filePath << std::endl;
                        return;
                    }
                } while (l.find_first_not_of(" \t\r\n") == std::string::npos);

                std::stringstream ss(l);
                for (int j = 0; j < jobs; ++j)
                {
                    int val = 0;
                    if (!(ss >> val))
                    {
                        std::cerr << "Error: failed to parse setup_time at machine=" << m << " prev=" << i
                                  << " curr=" << j << " from line: '" << l << "' in " << filePath << std::endl;
                        return;
                    }
                    setup_time[m][i][j] = val;
                }
            }
        }

        file.close();

        std::cout << "=== Instance loaded from: " << filePath << " ===" << std::endl;
        std::cout << "Jobs: " << jobs << ", Machines: " << machines << std::endl;
    }

    int getJobs() const { return jobs; }
    int getMachines() const { return machines; }

    int getProcTime(int job, int machine) const
    {
        return proc_time.at(job).at(machine);
    }

    int getSetupTime(int machine, int prevJob, int currJob) const
    {
        return setup_time.at(machine).at(prevJob).at(currJob);
    }

    const std::vector<std::vector<int>> &getProcTimes() const { return proc_time; }
    const std::vector<std::vector<std::vector<int>>> &getSetupTimes() const { return setup_time; }

    inline double computeMakespan(const std::vector<int> &seq) const
    {
        int n = (int)seq.size();
        if (n == 0)
            return 0.0;

        int m = machines;

        std::vector<std::vector<double>> completion(n, std::vector<double>(m, 0.0));

        for (int idx = 0; idx < n; ++idx)
        {
            int job = seq[idx];

            for (int mach = 0; mach < m; ++mach)
            {

                double prevMachineTime = (mach > 0) ? completion[idx][mach - 1] : 0.0;
                double prevJobTime = (idx > 0) ? completion[idx - 1][mach] : 0.0;

                int prevJob = (idx > 0) ? seq[idx - 1] : job;
                double setup = 0.0;
                if (idx > 0)
                {

                    setup = setup_time[mach][prevJob][job];
                }

                double proc = proc_time[job][mach];

                double start = std::max(prevMachineTime, prevJobTime + setup);
                completion[idx][mach] = start + proc;
            }
        }

        return completion[n - 1][m - 1];
    }
};
