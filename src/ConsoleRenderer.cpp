#include "ConsoleRenderer.h"
#include "ProcessStats.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <format>

ConsoleRenderer::ConsoleRenderer() {
    console_ = GetStdHandle(STD_OUTPUT_HANDLE);

    // Enable virtual terminal processing if available (Windows 10+)
    DWORD mode = 0;
    if (GetConsoleMode(console_, &mode)) {
        SetConsoleMode(console_, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
    }

    // Clear the entire screen once at startup
    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    if (GetConsoleScreenBufferInfo(console_, &csbi)) {
        DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
        DWORD written = 0;
        COORD origin{ 0, 0 };
        FillConsoleOutputCharacterW(console_, L' ', cells, origin, &written);
        FillConsoleOutputAttribute(console_, csbi.wAttributes, cells, origin, &written);
        SetConsoleCursorPosition(console_, origin);
    }
}

void ConsoleRenderer::WriteAt(SHORT x, SHORT y, const std::wstring& text) {
    COORD pos{ x, y };
    SetConsoleCursorPosition(console_, pos);
    DWORD written = 0;
    WriteConsoleW(console_, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
}

// Pad a string to a fixed width with spaces so previous longer lines are overwritten
static std::wstring PadToWidth(const std::wstring& s, size_t width) {
    if (s.size() >= width)
        return s.substr(0, width);
    return s + std::wstring(width - s.size(), L' ');
}

std::wstring ConsoleRenderer::FormatCumulative(uint64_t count) {
    if (count < 1024)
        return std::to_wstring(count) + L" B";
    if (count < 1024 * 1024) {
        std::wostringstream oss;
        oss << std::fixed << std::setprecision(1) << (count / 1024.0) << L" KB";
        return oss.str();
    }
    if (count < 1024 * 1024 * 1024) {
        std::wostringstream oss;
        oss << std::fixed << std::setprecision(1) << (count / (1024.0 * 1024.0)) << L" MB";
        return oss.str();
    }
    std::wostringstream oss;
    oss << std::fixed << std::setprecision(1) << (count / (1024.0 * 1024.0 * 1024.0)) << L" GB";
    return oss.str();
}

std::wstring ConsoleRenderer::FormatRate(double bps) {
    if (bps < 1024.0)
        return std::to_wstring(static_cast<uint64_t>(bps)) + L" B/s";
    if (bps < 1024.0 * 1024.0) {
        std::wostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bps / 1024.0) << L" KB/s";
        return oss.str();
    }
    if (bps < 1024.0 * 1024.0 * 1024.0) {
        std::wostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bps / (1024.0 * 1024.0)) << L" MB/s";
        return oss.str();
    }
    std::wostringstream oss;
    oss << std::fixed << std::setprecision(1) << (bps / (1024.0 * 1024.0 * 1024.0)) << L" GB/s";
    return oss.str();
}

std::wstring ConsoleRenderer::Truncate(const std::wstring& s, size_t max) {
    if (s.size() <= max) return s;
    if (max <= 3) return s.substr(0, max);
    return s.substr(0, max - 3) + L"...";
}

void ConsoleRenderer::HandleInput() {
    INPUT_RECORD rec{};
    DWORD read = 0;
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (PeekConsoleInputW(input, &rec, 1, &read) && read > 0) {
        ReadConsoleInputW(input, &rec, 1, &read);
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
            wchar_t ch = rec.Event.KeyEvent.uChar.UnicodeChar;
            if (ch == L'q' || ch == L'Q') {
                SetConsoleCursorPosition(console_, COORD{ 0, rows - 1 } );
                quit_ = true;
            } else if (ch == L'h' || ch == L'H')
                showTerminated_ = !showTerminated_;
            else if (ch == L'c' || ch == L'C')
                cumulativeMode_ = !cumulativeMode_;
            else if (ch == L'r' || ch == L'R')
                sortKey_ = SortKey::Read;
            else if (ch == L'w' || ch == L'W')
                sortKey_ = SortKey::Write;
            else if (ch == L'i' || ch == L'I')
                sortKey_ = SortKey::IOPS;
            else if (ch == L't' || ch == L'T')
                sortKey_ = SortKey::Total;
            else if (ch == L'p' || ch == L'P')
                sortKey_ = SortKey::Pid;
        }
    }
}

// Column widths (tune these to the largest values you expect)
constexpr int W_PID    = 8;
constexpr int W_NAME   = 20;
constexpr int W_OPS    = 8;   // a bit wider than 7 for larger counters
constexpr int W_BYTES  = 12;  // room for "1234.5 MB" etc.
constexpr int W_TOTAL  = 12;

SHORT ConsoleRenderer::WriteHeader(SHORT row, SHORT width) {
    auto writePadded = [&](const std::wstring& line) {
        WriteAt(0, row++, PadToWidth(line, static_cast<size_t>(width)));
    };

    std::wostringstream header;
    header << std::left  << std::setw(W_PID)  << L"PID"    << L" "
           << std::left  << std::setw(W_NAME) << L"Process" << L" "
           << std::right << std::setw(W_OPS)  << (cumulativeMode_ ? L"R_Ops"  : L"R_IOPS")  << L" "
           << std::right << std::setw(W_OPS)  << (cumulativeMode_ ? L"W_Ops"  : L"W_IOPS")  << L" "
           << std::right << std::setw(W_BYTES)<< (cumulativeMode_ ? L"Read"   : L"Read/s")  << L" "
           << std::right << std::setw(W_BYTES)<< (cumulativeMode_ ? L"Write"  : L"Write/s") << L" "
           << std::right << std::setw(W_TOTAL)<< (cumulativeMode_ ? L"Total"  : L"Total/s");

    writePadded(header.str());

    // Separator that matches the total width
    writePadded(std::wstring(W_PID + 1 + W_NAME + 1 + W_OPS + 1 + W_OPS + 1 +
                             W_BYTES + 1 + W_BYTES + 1 + W_TOTAL, L'-'));
    return row;
}


void ConsoleRenderer::Update(const ViewModel& view) {
    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    SHORT row = 0;
    if (GetConsoleScreenBufferInfo(console_, &csbi)) {
        cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        
        if ( cols <= 0 ) cols = csbi.dwSize.X;
        if ( rows <= 0 ) rows = csbi.dwSize.Y;
    }

    SHORT width = static_cast<SHORT>(std::min<int>(cols,200));
    auto writePadded = [&](const std::wstring& line) {
        WriteAt(0, row++, PadToWidth(line, static_cast<size_t>(width)));
    };

    writePadded(L"Volume: " + view.Volume);
    writePadded(L"Lost Events: " + std::to_wstring(view.Diags.LostEvents.load()));
    writePadded(L"Lost Buffers: " + std::to_wstring(view.Diags.LostBuffers.load()));
    writePadded(L"");

    row = WriteHeader(row, width);

    size_t maxRows = std::max<int>(1, rows - 12 );
    size_t shown = 0;
    size_t visibleCount = 0;

    for (const auto& r : view.Rows) {
        if (!showTerminated_ && r.Terminated)
            continue;
        ++visibleCount;
        if (shown >= maxRows)
            continue;

        std::wostringstream line;
        line << std::right << std::setw(W_PID)  << r.Identity.Pid << L" "
             << std::left  << std::setw(W_NAME) << Truncate(r.ProcessName, W_NAME) << L" ";

        if (cumulativeMode_) {
            line << std::right << std::setw(W_OPS)   << r.ReadOpsTotal  << L" "
                 << std::right << std::setw(W_OPS)   << r.WriteOpsTotal << L" "
                 << std::right << std::setw(W_BYTES) << FormatCumulative(r.ReadBytesTotal)  << L" "
                 << std::right << std::setw(W_BYTES) << FormatCumulative(r.WriteBytesTotal) << L" "
                 << std::right << std::setw(W_TOTAL) << FormatCumulative(r.ReadBytesTotal + r.WriteBytesTotal);
        } else {
            line << std::right << std::setw(W_OPS)   << r.ReadIOPS  << L" "
                 << std::right << std::setw(W_OPS)   << r.WriteIOPS << L" "
                 << std::right << std::setw(W_BYTES) << FormatRate(r.ReadPerSecond)  << L" "
                 << std::right << std::setw(W_BYTES) << FormatRate(r.WritePerSecond) << L" "
                 << std::right << std::setw(W_TOTAL) << FormatRate(r.ReadPerSecond + r.WritePerSecond);
        }
        writePadded(line.str());
        ++shown;
    }
    for (; shown < maxRows; ++shown)
        writePadded(L"--");

    writePadded(L"");
    std::wostringstream footer;
    footer << L"Processes: " << visibleCount
           << L" | DecodeErr: " << view.Diags.SchemaDecodeFailures.load()
           << L" | UnknownFO: " << view.Diags.UnknownFileObjects.load()
           << L" | Ev/s: " << view.EventsPerSec
           << L" | Session: " << (view.Owned ? L"Owned" : L"Attached");
    writePadded(footer.str());
    footer.str(L""); footer.clear();
    footer << L"Terminated: " << (showTerminated_ ? L"shown" : L"hidden")
           << L" | Mode: " << (cumulativeMode_ ? L"cumulative" : L"real-time");
    writePadded(footer.str());
    footer.str(L""); footer.clear();
    writePadded(L"Sort By: r Reads w Write i Iops t Total p PID ");
    if ( showTerminated_ ) {
        footer << L"Misc:    h hide exited ";
    } else {
        footer << L"Misc:    h show exited ";
    }
    if ( cumulativeMode_ ) {
        footer << L" c Real Time    q Quit";
    } else {
        footer << L" c Cumulative   q Quit";
    }
    writePadded(footer.str());
    footer.str(L""); footer.clear();

    SetConsoleCursorPosition(console_, COORD{ 0, 0 });
}


