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

enum class SortKey {
    Total,      // default
    Read,
    Write,
    IOPS,
    Pid
};

class ConsoleRenderer {
public:
    ConsoleRenderer();
    void Update(const ViewModel& view);
    void HandleInput();
    bool ShouldQuit() const { return quit_; }
    SortKey GetSortKey() const { return sortKey_; }

private:
    void WriteAt(SHORT x, SHORT y, const std::wstring& text);
    SHORT WriteHeader(SHORT row, SHORT width);
    std::wstring FormatCumulative(uint64_t count);
    std::wstring FormatRate(double bytesPerSec);
    std::wstring Truncate(const std::wstring& s, size_t maxChars);

    HANDLE console_ = INVALID_HANDLE_VALUE;
    SHORT rows = 24;
    SHORT cols = 80;
    bool quit_ = false;
    bool showTerminated_ = false;
    bool cumulativeMode_ = false;
    SortKey sortKey_ = SortKey::Total;
};



