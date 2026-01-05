#include "Schedule.hpp"
#include "Instance.hpp"
#include <algorithm>
#include <iostream>

void Schedule::emitFinalSlots(const Instance &instance) const
{
    const std::vector<int> &sequence = jobSequence;
    if (sequence.empty())
        return;

    int m = instance.getMachines();
    int n = (int)sequence.size();

    struct OpRec {
        int job;
        long long start;
        long long end;
        int setup_used;
    };

    std::vector<std::vector<OpRec>> schedule(m);
    std::vector<long long> machineTime(m, 0);

    // Obliczanie harmonogramu (identyczne z Twoją logiką)
    for (int idx = 0; idx < n; ++idx) {
        int job = sequence[idx];
        for (int mach = 0; mach < m; ++mach) {
            long long readyByMachine = (mach == 0) ? 0 : machineTime[mach - 1];
            long long readyBySequence = machineTime[mach];

            int prevJob = (idx == 0) ? -1 : sequence[idx - 1];
            int setupTime = (idx == 0 || prevJob == -1) ? 0 : instance.getSetupTime(mach, prevJob, job);

            long long startTime = std::max(readyByMachine, readyBySequence + (long long)setupTime);
            long long endTime = startTime + instance.getProcTime(job, mach);

            machineTime[mach] = endTime;
            schedule[mach].push_back({job, startTime, endTime, setupTime});
        }
    }

    // Wypisywanie danych w formacie zrozumiałym dla GUI
    // Używamy iter=0 dla wyniku końcowego
    for (int mIdx = 0; mIdx < (int)schedule.size(); ++mIdx) {
        for (const auto &op : schedule[mIdx]) {
            std::cout << "SLOT;iter=0;machine=" << mIdx
                      << ";job=" << op.job
                      << ";start=" << op.start
                      << ";end=" << op.end
                      << ";setup=" << op.setup_used << std::endl;
        }
    }
    
    // Kluczowy sygnał zakończenia ramki danych
    std::cout << "FRAME_END;iter=0" << std::endl;
    std::cout.flush(); // Wymuszenie wysłania danych do Pythona
}