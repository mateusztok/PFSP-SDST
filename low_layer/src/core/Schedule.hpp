#pragma once
#include <vector>
#include <iostream>
#include <algorithm>
class Instance;

class Schedule
{
private:
    std::vector<int> jobSequence;

public:
    Schedule() = default;
    Schedule(const std::vector<int> &seq) : jobSequence(seq) {}
    Schedule(int jobs)
    {
        jobSequence.resize(jobs);
        for (int i = 0; i < jobs; ++i)
            jobSequence[i] = i;
    }

    const std::vector<int> &getJobSequence() const { return jobSequence; }

    void print() const
    {
        for (int j : jobSequence)
            std::cout << j << " ";
        std::cout << std::endl;
    }

    void emitFinalSlots(const class Instance &instance) const;
};
