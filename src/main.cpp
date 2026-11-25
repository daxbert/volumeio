#include <algorithm>
#include "EtwSession.h"
#include "EventDispatch.h"
#include "FileObjectMap.h"
#include "ProcessStats.h"
#include "ConsoleRenderer.h"
#include "RateCalculator.h"
#include "VolumeMapper.h"
#include "Diagnostics.h"
#include "PathNormalizer.h"
#include <windows.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

static std::atomic<bool> g_shutdown{false};

BOOL WINAPI ConsoleHandler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_CLOSE_EVENT || type == CTRL_BREAK_EVENT) {
        g_shutdown = true;
        return TRUE;
    }
    return FALSE;
}

bool IsAdministrator() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin != FALSE;
}

int wmain(int argc, wchar_t* argv[]) {
    if (!IsAdministrator()) {
        std::wcerr << L"ERROR:\nAdministrator privileges required\nfor kernel ETW sessions.\n";
        return 1;
    }

    if (argc != 2 || wcslen(argv[1]) < 2 || argv[1][1] != L':') {
        std::wcerr << L"ERROR:\nInvalid drive specification.\nExpected format: X:\n";
        return 1;
    }
    wchar_t drive = towupper(argv[1][0]);
    std::wstring root = std::wstring(1, drive) + L":\\";
    if (GetDriveTypeW(root.c_str()) == DRIVE_NO_ROOT_DIR) {
        std::wcerr << L"ERROR:\nInvalid drive specification.\nExpected format: X:\n";
        return 1;
    }

    VolumeMapper mapper;
    std::wstring ntPath = mapper.GetNtPathForDrive(drive);
    if (ntPath.empty()) {
        std::wcerr << L"ERROR:\nCould not resolve NT path for drive.\n";
        return 1;
    }
    ntPath = NormalizePath(ntPath);

    DiagnosticCounters diags;
    FileObjectMap fileMap(diags, ntPath);
    ProcessStats stats(diags);
    EventDispatch dispatch(fileMap, stats, diags, ntPath);
    EtwSession session(diags);

    session.SetEventCallback([&](PEVENT_RECORD e) { dispatch.OnEvent(e); });

    const ULONG requiredFlags =
        EVENT_TRACE_FLAG_PROCESS |
        EVENT_TRACE_FLAG_DISK_FILE_IO |
        EVENT_TRACE_FLAG_FILE_IO |
        EVENT_TRACE_FLAG_FILE_IO_INIT;

    if (!session.Start(requiredFlags)) {
        std::wcerr << L"ERROR:\nFailed to start or attach to NT Kernel Logger.\n";
        return 1;
    }

    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    ConsoleRenderer renderer;
    session.Run();

    LARGE_INTEGER freq, prevQpc;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prevQpc);
    std::vector<ProcessStatsEntry> previousSnapshot;

    while (!g_shutdown && !renderer.ShouldQuit()) {
        renderer.HandleInput();

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double delta = static_cast<double>(now.QuadPart - prevQpc.QuadPart) / freq.QuadPart;
        if (delta >= 1.0 || delta <= 0) {
            auto snapshot = stats.GetSnapshot();
            std::vector<RateView> views;
            RateCalculator::Calculate(snapshot, previousSnapshot, delta <= 0 ? 1.0 : delta, views);

            // Sort top 50 (simplified; full sort by Total/s etc. according to hotkey)
            std::sort(views.begin(), views.end(), [](const RateView& a, const RateView& b) {
                return (a.ReadPerSecond + a.WritePerSecond) > (b.ReadPerSecond + b.WritePerSecond);
            });
            if (views.size() > 50) views.resize(50);

            ViewModel vm{std::wstring(1, drive) + L":", views, diags, session.IsOwned(), session.GetEnableFlags(), diags.EventsThisSecond.load()};
            renderer.Update(vm);
            diags.ResetSecond();
            prevQpc = now;

            fileMap.ReapOrphans(); // every ~1s is fine; can throttle to 60s
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    session.Stop();
    return 0;
}
