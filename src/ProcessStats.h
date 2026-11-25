#pragma once
#include "ProcessIdentity.h"
#include "Diagnostics.h"
#include <unordered_map>
#include <shared_mutex>
#include <string>
#include <optional>

struct ProcessStatsEntry {
    ProcessIdentity Identity;
    std::wstring ProcessName;
    uint64_t ReadBytesTotal = 0;
    uint64_t WriteBytesTotal = 0;
    uint64_t ReadBytesLastSecond = 0;
    uint64_t WriteBytesLastSecond = 0;
    uint32_t ReadIOPSLastSecond = 0;
    uint32_t WriteIOPSLastSecond = 0;
    uint64_t LastActivityTick = 0;
    bool Terminated = false;
    bool NameResolutionAttempted = false;
};

class ProcessStats {
public:
    explicit ProcessStats(DiagnosticCounters& diags);

    void OnProcessStart(DWORD pid, const std::wstring& imageFileName, FILETIME createTime);
    void OnProcessEnd(DWORD pid);
    void AddIO(const ProcessIdentity& id, uint64_t bytes, bool isWrite);
    void UpdateRates(double deltaSeconds);
    std::vector<ProcessStatsEntry> GetSnapshot() const;

    // For tests
    std::optional<std::wstring> GetName(const ProcessIdentity& id) const;

private:
    std::wstring ResolveName(DWORD pid);
    DiagnosticCounters& diags_;
    mutable std::shared_mutex mutex_;
    std::unordered_map<ProcessIdentity, ProcessStatsEntry> map_;
    static constexpr size_t MaxEntries = 100000;
};
