#pragma once
#include "PathNormalizer.h"
#include "Diagnostics.h"
#include <unordered_map>
#include <shared_mutex>
#include <chrono>
#include <cstdint>
#include <string>

struct FileObjectInfo {
    std::wstring Path;
    bool MatchesTargetVolume = false;
    std::chrono::steady_clock::time_point LastUpdate{};
    bool Active = true;
};

class FileObjectMap {
public:
    explicit FileObjectMap(DiagnosticCounters& diags, const std::wstring& targetNtPath);

    void OnCreate(uint64_t fileObject, const std::wstring& openPath);
    void OnName(uint64_t fileObject, const std::wstring& fileName);
    bool OnReadWrite(uint64_t fileObject, uint64_t ioSize, bool isWrite, DWORD processId, FILETIME creationTime);
    void OnCleanup(uint64_t fileObject);
    void OnClose(uint64_t fileObject);
    void ReapOrphans();

    // For tests
    bool Has(uint64_t fo) const;
    bool Matches(uint64_t fo) const;

private:
    DiagnosticCounters& diags_;
    std::wstring targetNtPath_;
    mutable std::shared_mutex mutex_;
    std::unordered_map<uint64_t, FileObjectInfo> map_;
};
