#include "ConsoleRenderer.h"
#include "ProcessStats.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

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
void ConsoleRenderer::Update(const ViewModel& view) {
    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    SHORT width = 120;
    if (GetConsoleScreenBufferInfo(console_, &csbi))
        width = std::min<SHORT>(csbi.dwSize.X, 120);

    SHORT row = 0;
    auto writePadded = [&](const std::wstring& line) {
        WriteAt(0, row++, PadToWidth(line, static_cast<size_t>(width)));
    };

    writePadded(L"Volume: " + view.Volume);
    writePadded(L"Lost Events: " + std::to_wstring(view.Diags.LostEvents.load()));
    writePadded(L"Lost Buffers: " + std::to_wstring(view.Diags.LostBuffers.load()));
    writePadded(L"");

    writePadded(L"PID     Process             R_IOPS  W_IOPS  Read/s     Write/s    Total/s");
    writePadded(L"-------------------------------------------------------------------------");

    // Filter for display only
    std::vector<RateView> displayRows;
    displayRows.reserve(view.Rows.size());
    for (const auto& r : view.Rows) {
        // RateView does not currently carry Terminated; we need it.
        // See note below.
        displayRows.push_back(r);
    }

    const size_t maxRows = 50;
    size_t shown = 0;
    for (const auto& r : displayRows) {
        if (shown >= maxRows) break;
        if ( !showTerminated_ && r.Terminated )
            continue;

        std::wostringstream line;
        line << std::setw(8) << r.Identity.Pid << L" "
             << std::left << std::setw(20) << Truncate(r.ProcessName, 20) << L" "
             << std::setw(7) << r.ReadIOPS << L" "
             << std::setw(7) << r.WriteIOPS << L" "
             << std::setw(10) << FormatRate(r.ReadPerSecond) << L" "
             << std::setw(10) << FormatRate(r.WritePerSecond) << L" "
             << FormatRate(r.ReadPerSecond + r.WritePerSecond);
        writePadded(line.str());
        ++shown;
    }
    for (; shown < maxRows; ++shown)
        writePadded(L"");

    writePadded(L"");
    std::wostringstream footer;
    footer << L"Processes: " << view.Rows.size()
           << L" | DecodeErr: " << view.Diags.SchemaDecodeFailures.load()
           << L" | UnknownFO: " << view.Diags.UnknownFileObjects.load()
           << L" | Ev/s: " << view.EventsPerSec
           << L" | Session: " << (view.Owned ? L"Owned" : L"Attached")
           << L" | Terminated: " << (showTerminated_ ? L"shown" : L"hidden");
    writePadded(footer.str());
    writePadded(L"Hotkeys: r Read/s  w Write/s  i IOPS  t Total/s  p PID  h Hide/Show exited  q Quit");

    SetConsoleCursorPosition(console_, COORD{ 0, row });
}

void ConsoleRenderer::HandleInput() {
    INPUT_RECORD rec{};
    DWORD read = 0;
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    if (PeekConsoleInputW(input, &rec, 1, &read) && read > 0) {
        ReadConsoleInputW(input, &rec, 1, &read);
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
            wchar_t ch = rec.Event.KeyEvent.uChar.UnicodeChar;
            if (ch == L'q' || ch == L'Q')
                quit_ = true;
            else if ( ch == L'h' || ch == L'H' )
                showTerminated_ = !showTerminated_;
        }
    }
}


