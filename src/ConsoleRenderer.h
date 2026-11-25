#pragma once
#include "RateCalculator.h"
#include "Diagnostics.h"
#include <string>
#include <vector>
#include <windows.h>

struct ViewModel {
    std::wstring Volume;
    std::vector<RateView> Rows;
    DiagnosticCounters& Diags;
    bool Owned = false;
    ULONG EnableFlags = 0;
    uint64_t EventsPerSec = 0;
};

class ConsoleRenderer {
public:
    ConsoleRenderer();
    void Update(const ViewModel& view);
    void HandleInput();
    bool ShouldQuit() const { return quit_; }

private:
    void WriteAt(SHORT x, SHORT y, const std::wstring& text);
    std::wstring FormatRate(double bytesPerSec);
    std::wstring Truncate(const std::wstring& s, size_t maxChars);

    HANDLE console_ = INVALID_HANDLE_VALUE;
    bool quit_ = false;
};


