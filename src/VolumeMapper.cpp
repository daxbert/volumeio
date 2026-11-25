#include "VolumeMapper.h"
#include <windows.h>
#include <unordered_map>
#include <mutex>

static std::mutex g_mutex;
static std::unordered_map<wchar_t, std::wstring> g_driveToNt;
static std::unordered_map<std::wstring, wchar_t> g_ntToDrive;

void VolumeMapper::RefreshMappings() {
    std::lock_guard lock(g_mutex);
    g_driveToNt.clear();
    g_ntToDrive.clear();

    wchar_t drives[256]{};
    DWORD len = GetLogicalDriveStringsW(sizeof(drives) / sizeof(wchar_t), drives);
    for (const wchar_t* p = drives; *p; p += wcslen(p) + 1) {
        if (p[1] == L':') {
            wchar_t letter = towupper(p[0]);
            wchar_t device[MAX_PATH]{};
            if (QueryDosDeviceW(std::wstring(1, letter).c_str(), device, MAX_PATH)) {
                std::wstring nt = device;
                g_driveToNt[letter] = nt;
                g_ntToDrive[nt] = letter;
            }
        }
    }
}

std::wstring VolumeMapper::GetNtPathForDrive(wchar_t driveLetter) {
    std::lock_guard lock(g_mutex);
    if (g_driveToNt.empty()) RefreshMappings();
    auto it = g_driveToNt.find(towupper(driveLetter));
    return it != g_driveToNt.end() ? it->second : L"";
}

std::optional<wchar_t> VolumeMapper::GetDriveForNtPath(const std::wstring& ntPath) {
    std::lock_guard lock(g_mutex);
    if (g_ntToDrive.empty()) RefreshMappings();
    auto it = g_ntToDrive.find(ntPath);
    if (it != g_ntToDrive.end()) return it->second;
    return std::nullopt;
}
