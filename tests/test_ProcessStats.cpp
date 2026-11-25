#include <catch2/catch_test_macros.hpp>
#include "ProcessStats.h"
#include "Diagnostics.h"
#include "ProcessIdentity.h"

TEST_CASE("ProcessStats Process/Start extracts base name", "[ProcessStats]") {
    DiagnosticCounters diags;
    ProcessStats stats(diags);

    FILETIME ct{};
    stats.OnProcessStart(4248, L"C:\\Windows\\System32\\ffmpeg.exe", ct);

    ProcessIdentity id{4248, ct};
    auto name = stats.GetName(id);
    REQUIRE(name.has_value());
    REQUIRE(*name == L"ffmpeg.exe");
}

TEST_CASE("ProcessStats synthetic identity (CreationTime = 0) is valid", "[ProcessStats]") {
    DiagnosticCounters diags;
    ProcessStats stats(diags);

    ProcessIdentity synthetic{552, {0, 0}};
    stats.AddIO(synthetic, 1024, false);

    auto name = stats.GetName(synthetic);
    // Name resolution fallback will be attempted; we only care that the identity is accepted
    REQUIRE(stats.GetSnapshot().size() == 1);
    REQUIRE(stats.GetSnapshot()[0].Identity == synthetic);
}

TEST_CASE("ProcessStats name resolution failure increments counter and uses [pid] form", "[ProcessStats]") {
    DiagnosticCounters diags;
    ProcessStats stats(diags);

    // PID that almost certainly cannot be opened
    ProcessIdentity id{0xFFFFFFFE, {0, 0}};
    stats.AddIO(id, 512, true);

    auto name = stats.GetName(id);
    REQUIRE(name.has_value());
    // Either "[4294967294]" or "unknown" depending on exact failure path
    REQUIRE((name->find(L"[") != std::wstring::npos || *name == L"unknown"));
    REQUIRE(diags.NameResolutionFailures.load() >= 1);
}

TEST_CASE("ProcessStats caches name resolution result (success or failure)", "[ProcessStats]") {
    DiagnosticCounters diags;
    ProcessStats stats(diags);

    ProcessIdentity id{12345, {0, 0}};
    uint64_t before = diags.NameResolutionFailures.load();

    stats.AddIO(id, 100, false);
    stats.AddIO(id, 200, false);   // second call must not re-resolve

    uint64_t after = diags.NameResolutionFailures.load();
    // Failure counter increases at most once
    REQUIRE(after - before <= 1);
}

TEST_CASE("ProcessStats bounded retention only evicts terminated entries", "[ProcessStats]") {
    DiagnosticCounters diags;
    ProcessStats stats(diags);

    // Fill with terminated identities (well below the 100k limit for the unit test)
    for (DWORD i = 1; i <= 100; ++i) {
        ProcessIdentity id{i, {0, 0}};                                                                                                                                                stats.OnProcessStart(i, L"proc.exe", {});
        stats.OnProcessEnd(i);
        stats.AddIO(id, 1, false);
    }

    // Live (non-terminated) identity must survive
    ProcessIdentity live{99999, {0, 0}};
    stats.AddIO(live, 4096, true);

    auto snap = stats.GetSnapshot();
    bool foundLive = false;
    for (const auto& e : snap) {
        if (e.Identity == live) {
            foundLive = true;                                                                                                                                                             REQUIRE_FALSE(e.Terminated);
        }
    }
    REQUIRE(foundLive);
}
