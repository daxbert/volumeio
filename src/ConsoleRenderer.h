#pragma once
#include "RateCalculator.h"
#include "Diagnostics.h"
#include <string>
#include <vector>
#include <windows.h>

struct ViewModel {
    std::wstring Volume;
    std::vector<RateView> Rows; // already sorted top 50
    DiagnosticCounters& Diags;
    bool Owned;
    ULONG EnableFlags;
    uint64_t EventsPerSec;
};

class ConsoleRenderer {
public:
    ConsoleRenderer();
    void Update(const ViewModel& view);
    void HandleInput(); // for hotkeys and resize
    bool ShouldQuit() const { return quit_; }

private:
    void WriteAt(SHORT x, SHORT y, const std::wstring& text);
    std::wstring FormatRate(double bytesPerSec);
    std::wstring Truncate(const std::wstring& s, size_t maxChars);

    HANDLE console_ = INVALID_HANDLE_VALUE;
    bool quit_ = false;
    enum class SortKey { Total, Read, Write, IOPS, Pid } sort_ = SortKey::Total;
};
