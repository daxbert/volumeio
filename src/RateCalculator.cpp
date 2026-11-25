#include "RateCalculator.h"
#include <algorithm>

void RateCalculator::Calculate(const std::vector<ProcessStatsEntry>& current,
                               std::vector<ProcessStatsEntry>& previous,
                               double deltaSeconds,
                               std::vector<RateView>& outViews) {
    if (deltaSeconds <= 0) deltaSeconds = 1.0;

    outViews.clear();
    outViews.reserve(current.size());

    // Build previous map for lookup
    std::unordered_map<ProcessIdentity, ProcessStatsEntry> prevMap;
    for (const auto& p : previous) prevMap[p.Identity] = p;

    for (const auto& cur : current) {
        RateView view;
        view.Identity = cur.Identity;
        view.ProcessName = cur.ProcessName.empty() ? L"unknown" : cur.ProcessName;

        auto it = prevMap.find(cur.Identity);
        uint64_t prevRead = it != prevMap.end() ? it->second.ReadBytesTotal : 0;
        uint64_t prevWrite = it != prevMap.end() ? it->second.WriteBytesTotal : 0;

        view.ReadPerSecond = static_cast<double>(cur.ReadBytesTotal - prevRead) / deltaSeconds;
        view.WritePerSecond = static_cast<double>(cur.WriteBytesTotal - prevWrite) / deltaSeconds;
        view.ReadIOPS = cur.ReadIOPSLastSecond; // placeholder; real IOPS tracked separately if needed
        view.WriteIOPS = cur.WriteIOPSLastSecond;
        view.TotalBytesLast = cur.ReadBytesTotal + cur.WriteBytesTotal;

        outViews.push_back(view);
    }

    previous = current; // update for next interval
}
