#pragma once
#include "ProcessStats.h"
#include <vector>
#include <cstdint>

struct RateView {
    ProcessIdentity Identity;
    std::wstring ProcessName;
    double ReadPerSecond = 0;
    double WritePerSecond = 0;
    uint32_t ReadIOPS = 0;
    uint32_t WriteIOPS = 0;
    uint64_t TotalBytesLast = 0;
};

class RateCalculator {
public:
    static void Calculate(const std::vector<ProcessStatsEntry>& current,
                          std::vector<ProcessStatsEntry>& previous,
                          double deltaSeconds,
                          std::vector<RateView>& outViews);
};
