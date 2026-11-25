#include <catch2/catch_test_macros.hpp>
#include "VolumeMapper.h"
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>

TEST_CASE("VolumeMapper round-trip on real volumes", "[VolumeMapper]") {
    VolumeMapper mapper;
    mapper.RefreshMappings();

    wchar_t drives[512]{};
    DWORD len = GetLogicalDriveStringsW(sizeof(drives) / sizeof(wchar_t), drives);
    REQUIRE(len > 0);

    std::vector<wchar_t> letters;
    for (const wchar_t* p = drives; *p; p += wcslen(p) + 1) {
        if (p[1] == L':') {
            letters.push_back(towupper(p[0]));
        }
    }
    REQUIRE(letters.size() >= 2);   // Spec requires at least two real volumes

    for (size_t i = 0; i < 2 && i < letters.size(); ++i) {
        wchar_t letter = letters[i];
        std::wstring nt = mapper.GetNtPathForDrive(letter);
        REQUIRE_FALSE(nt.empty());

        auto roundTrip = mapper.GetDriveForNtPath(nt);
        REQUIRE(roundTrip.has_value());
        REQUIRE(*roundTrip == letter);
    }
}

TEST_CASE("VolumeMapper invalid drive letter", "[VolumeMapper]") {
    VolumeMapper mapper;
    std::wstring result = mapper.GetNtPathForDrive(L'Z');  // Z: is almost never present; any invalid letter is fine
    // Even if Z: happens to exist on the test machine, we also test a clearly invalid character
    std::wstring invalid = mapper.GetNtPathForDrive(L'@');
    REQUIRE(invalid.empty());

    // Explicit empty / out-of-range letter
    REQUIRE(mapper.GetNtPathForDrive(L'\0').empty());
    REQUIRE(mapper.GetNtPathForDrive(L'1').empty());
}
