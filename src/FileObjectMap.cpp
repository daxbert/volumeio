#include "FileObjectMap.h"

FileObjectMap::FileObjectMap(DiagnosticCounters& diags, const std::wstring& targetNtPath)
    : diags_(diags), targetNtPath_(NormalizePath(targetNtPath)) {}

void FileObjectMap::OnCreate(uint64_t fileObject, const std::wstring& openPath) {
    std::unique_lock lock(mutex_);
    auto& info = map_[fileObject];
    info.Path = NormalizePath(openPath);
    info.MatchesTargetVolume = MatchesTargetVolume(info.Path, targetNtPath_);
    info.LastUpdate = std::chrono::steady_clock::now();
    info.Active = true;
    if (!info.MatchesTargetVolume && !info.Path.empty())
        diags_.VolumeMatchFailures.fetch_add(1, std::memory_order_relaxed);
}

void FileObjectMap::OnName(uint64_t fileObject, const std::wstring& fileName) {
    std::unique_lock lock(mutex_);
    auto it = map_.find(fileObject);
    if (it == map_.end()) return;
    it->second.Path = NormalizePath(fileName);
    it->second.MatchesTargetVolume = MatchesTargetVolume(it->second.Path, targetNtPath_);
    it->second.LastUpdate = std::chrono::steady_clock::now();
    if (!it->second.MatchesTargetVolume && !it->second.Path.empty())
        diags_.VolumeMatchFailures.fetch_add(1, std::memory_order_relaxed);
}

bool FileObjectMap::OnReadWrite(uint64_t fileObject, uint64_t /*ioSize*/, bool /*isWrite*/) {
    std::shared_lock lock(mutex_);
    auto it = map_.find(fileObject);
    if (it == map_.end() || !it->second.Active) {
        diags_.UnknownFileObjects.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    return it->second.MatchesTargetVolume;
}

void FileObjectMap::OnCleanup(uint64_t fileObject) {
    std::shared_lock lock(mutex_);
    if (map_.find(fileObject) == map_.end())
        diags_.OrphanCleanupEvents.fetch_add(1, std::memory_order_relaxed);
}

void FileObjectMap::OnClose(uint64_t fileObject) {
    std::unique_lock lock(mutex_);
    auto it = map_.find(fileObject);
    if (it == map_.end()) {
        diags_.OrphanCloseEvents.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    it->second.Active = false;
    it->second.LastUpdate = std::chrono::steady_clock::now();
}

void FileObjectMap::ReapOrphans() {
    auto now = std::chrono::steady_clock::now();
    auto threshold = now - std::chrono::minutes(5);
    std::unique_lock lock(mutex_);
    for (auto it = map_.begin(); it != map_.end(); ) {
        if (!it->second.Active && it->second.LastUpdate < threshold)
            it = map_.erase(it);
        else
            ++it;
    }
}

bool FileObjectMap::Has(uint64_t fo) const {
    std::shared_lock lock(mutex_);
    return map_.count(fo) != 0;
}

bool FileObjectMap::Matches(uint64_t fo) const {
    std::shared_lock lock(mutex_);
    auto it = map_.find(fo);
    return it != map_.end() && it->second.MatchesTargetVolume;
}


