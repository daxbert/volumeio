#include "VolumeMapper.h"
#include <windows.h>
#include <cwctype>

void VolumeMapper::RefreshMappingsUnlocked() {
    driveToNt_.clear();
    ntToDrive_.clear();

    wchar_t drives[512]{};
    DWORD len = GetLogicalDriveStringsW(sizeof(drives) / sizeof(wchar_t), drives);
    if (len == 0 || len >= sizeof(drives) / sizeof(wchar_t))
        return;

    for (const wchar_t* p = drives; *p; p += wcslen(p) + 1) {
        if (p[1] == L':') {
            wchar_t letter = static_cast<wchar_t>(towupper(p[0]));
            wchar_t dosName[3] = { letter, L':', L'\0' };
            wchar_t device[MAX_PATH]{};
            if (QueryDosDeviceW(dosName, device, MAX_PATH)) {
                std::wstring nt(device);
                driveToNt_[letter] = nt;
                ntToDrive_[nt] = letter;
            }
        }
    }
}

void VolumeMapper::RefreshMappings() {
    std::lock_guard lock(mutex_);
    RefreshMappingsUnlocked();
}

std::wstring VolumeMapper::GetNtPathForDrive(wchar_t driveLetter) {
    std::lock_guard lock(mutex_);
    if (driveToNt_.empty())
        RefreshMappingsUnlocked();
    auto it = driveToNt_.find(static_cast<wchar_t>(towupper(driveLetter)));
    return it != driveToNt_.end() ? it->second : L"";
}

std::optional<wchar_t> VolumeMapper::GetDriveForNtPath(const std::wstring& ntPath) {
    std::lock_guard lock(mutex_);
    if (ntToDrive_.empty())
        RefreshMappingsUnlocked();
    auto it = ntToDrive_.find(ntPath);
    return it != ntToDrive_.end() ? std::optional<wchar_t>(it->second) : std::nullopt;
}


