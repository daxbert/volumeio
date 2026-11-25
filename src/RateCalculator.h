#pragma once
#include "ProcessIdentity.h"
#include <vector>
#include <string>
#include <cstdint>

struct ProcessStatsEntry;   // forward declaration – full definition lives in ProcessStats.h

struct RateView {
    ProcessIdentity Identity;
    std::wstring ProcessName;
    double ReadPerSecond = 0;
    double WritePerSecond = 0;
    uint32_t ReadIOPS = 0;
    uint32_t WriteIOPS = 0;
    uint64_t TotalBytesLast = 0;
    uint64_t ReadBytesTotal = 0;        
    uint64_t WriteBytesTotal = 0;      
    uint64_t ReadOpsTotal = 0;        
    uint64_t WriteOpsTotal = 0;      
    bool Terminated = false; 
};

class RateCalculator {
public:
    static void Calculate(const std::vector<ProcessStatsEntry>& current,
                          std::vector<ProcessStatsEntry>& previous,
                          double deltaSeconds,
                          std::vector<RateView>& outViews);
};

