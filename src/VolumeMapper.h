#pragma once
#include <string>
#include <optional>

class VolumeMapper {
public:
    std::wstring GetNtPathForDrive(wchar_t driveLetter);
    std::optional<wchar_t> GetDriveForNtPath(const std::wstring& ntPath);
    void RefreshMappings();
};
