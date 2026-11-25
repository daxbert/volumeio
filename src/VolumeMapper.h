#pragma once
#include <string>
#include <optional>
#include <unordered_map>
#include <mutex>

class VolumeMapper {
public:
    std::wstring GetNtPathForDrive(wchar_t driveLetter);
    std::optional<wchar_t> GetDriveForNtPath(const std::wstring& ntPath);
    void RefreshMappings();

private:
    void RefreshMappingsUnlocked();   // must be called while holding mutex_
    std::mutex mutex_;
    std::unordered_map<wchar_t, std::wstring> driveToNt_;
    std::unordered_map<std::wstring, wchar_t> ntToDrive_;
};


