#include "ProcessStats.h"
#include <windows.h>

ProcessStats::ProcessStats(DiagnosticCounters& diags) : diags_(diags) {}

static std::wstring CleanProcessName(const std::wstring& raw) {
    std::wstring clean;
    clean.reserve(raw.size());
    for (wchar_t ch : raw) {
        // Keep printable BMP characters; drop controls, surrogates, and private-use garbage
        if (ch >= 32 && ch < 0xD800)
            clean.push_back(ch);
    }
    // Trim trailing dots/spaces that sometimes appear from truncated buffers
    while (!clean.empty() && (clean.back() == L'.' || clean.back() == L' '))
        clean.pop_back();

    // Replace non-ASCII characters with '#' after the above extraction
    for (wchar_t& ch : clean) {
        if (ch >= 0x80)
            ch = L'#';
    }
    return clean.empty() ? L"unknown" : clean;
}

void ProcessStats::OnProcessStart(DWORD pid, const std::wstring& imageFileName, FILETIME createTime) {
    ProcessIdentity id{pid, createTime};
    std::wstring base = imageFileName;
    auto pos = base.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        base = base.substr(pos + 1);

    std::unique_lock lock(mutex_);
    auto& entry = map_[id];
    entry.Identity = id;
    entry.ProcessName = CleanProcessName(base);
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

    if (isWrite) {
        entry.WriteBytesTotal += bytes;
        entry.WriteOpsTotal ++;
    } else {
        entry.ReadBytesTotal += bytes;
        entry.ReadOpsTotal ++;
    }
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
        std::wstring full(buf, size);          // size is the length without null
        auto pos = full.find_last_of(L"\\/");
        std::wstring base = (pos != std::wstring::npos) ? full.substr(pos + 1) : full;
        return CleanProcessName(base);
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

