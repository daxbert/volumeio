#include <catch2/catch_test_macros.hpp>
#include "PathNormalizer.h"
                                                                                                                    
TEST_CASE("NormalizePath basic", "[PathNormalizer]") {
    REQUIRE(NormalizePath(L"\\Device\\HarddiskVolume7\\foo\\bar") == L"\\DEVICE\\HARDDISKVOLUME7\\FOO\\BAR");

    auto p = NormalizePath(L"\\Device\\HarddiskVolume7\\foo\\bar\\");
    REQUIRE((p.empty() || p.back() != L'\\'));

    auto root = NormalizePath(L"\\Device\\HarddiskVolume7\\");
    REQUIRE(root == L"\\DEVICE\\HARDDISKVOLUME7"); 
}

TEST_CASE("MatchesTargetVolume", "[PathNormalizer]") {
    REQUIRE(MatchesTargetVolume(L"\\Device\\HarddiskVolume7\\foo", L"\\Device\\HarddiskVolume7") == true);
    REQUIRE(MatchesTargetVolume(L"\\Device\\HarddiskVolume7", L"\\Device\\HarddiskVolume7") == true);
    REQUIRE(MatchesTargetVolume(L"\\Device\\HarddiskVolume70", L"\\Device\\HarddiskVolume7") == false);
    REQUIRE(MatchesTargetVolume(L"", L"\\Device\\HarddiskVolume7") == false);
}


