#include "ProcessStats.h"
#include <windows.h>

ProcessStats::ProcessStats(DiagnosticCounters& diags) : diags_(diags) {}

void ProcessStats::OnProcessStart(DWORD pid, const std::wstring& imageFileName, FILETIME createTime) {
    ProcessIdentity id{pid, createTime};
    std::wstring base = imageFileName;
    auto pos = base.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        base = base.substr(pos + 1);

    std::unique_lock lock(mutex_);
    auto& entry = map_[id];
    entry.Identity = id;
    entry.ProcessName = base.empty() ? L"unknown" : base;
    entry.Terminated = false;
    entry.NameResolutionAttempted = true;
}

void ProcessStats::OnProcessEnd(DWORD pid) {
    std::unique_lock lock(mutex_);
    for (auto& [id, entry] : map_) {
        if (id.Pid == pid)
            entry.Terminated = true;
    }
}

void ProcessStats::AddIO(const ProcessIdentity& id, uint64_t bytes, bool isWrite) {
    std::unique_lock lock(mutex_);
    if (map_.size() >= MaxEntries) {
        for (auto it = map_.begin(); it != map_.end(); ) {
            if (it->second.Terminated) {
                it = map_.erase(it);
                if (map_.size() < MaxEntries * 9 / 10)
                    break;
            } else {
                ++it;
            }
        }
    }
    auto& entry = map_[id];
    entry.Identity = id;
    if (entry.ProcessName.empty())
        entry.ProcessName = ResolveName(id.Pid);

    if (isWrite)
        entry.WriteBytesTotal += bytes;
    else
        entry.ReadBytesTotal += bytes;
    entry.LastActivityTick = GetTickCount64();
}

std::wstring ProcessStats::ResolveName(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) {
        diags_.NameResolutionFailures.fetch_add(1, std::memory_order_relaxed);
        return L"[" + std::to_wstring(pid) + L"]";
    }
    wchar_t buf[MAX_PATH]{};
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameW(h, 0, buf, &size)) {
        CloseHandle(h);
        std::wstring full = buf;
        auto pos = full.find_last_of(L"\\/");
        return pos != std::wstring::npos ? full.substr(pos + 1) : full;
    }
    CloseHandle(h);
    diags_.NameResolutionFailures.fetch_add(1, std::memory_order_relaxed);
    return L"[" + std::to_wstring(pid) + L"]";
}

std::vector<ProcessStatsEntry> ProcessStats::GetSnapshot() const {
    std::shared_lock lock(mutex_);
    std::vector<ProcessStatsEntry> result;
    result.reserve(map_.size());
    for (const auto& [id, entry] : map_)
        result.push_back(entry);
    return result;
}

std::optional<std::wstring> ProcessStats::GetName(const ProcessIdentity& id) const {
    std::shared_lock lock(mutex_);
    auto it = map_.find(id);
    if (it != map_.end())
        return it->second.ProcessName;
    return std::nullopt;
}

