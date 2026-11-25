#include "ConsoleRenderer.h"
#include "ProcessStats.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

ConsoleRenderer::ConsoleRenderer() {
    console_ = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleMode(console_, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
}

void ConsoleRenderer::WriteAt(SHORT x, SHORT y, const std::wstring& text) {
    COORD pos{x, y};
    SetConsoleCursorPosition(console_, pos);
    DWORD written = 0;
    WriteConsoleW(console_, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
}

std::wstring ConsoleRenderer::FormatRate(double bps) {
    if (bps < 1024) return std::to_wstring(static_cast<uint64_t>(bps)) + L" B/s";
    if (bps < 1024 * 1024) return std::to_wstring(bps / 1024.0).substr(0, 4) + L" KB/s";
    if (bps < 1024LL * 1024 * 1024) {
        std::wostringstream oss;
        oss << std::fixed << std::setprecision(1) << (bps / (1024.0 * 1024)) << L" MB/s";
        return oss.str();
    }
    std::wostringstream oss;
    oss << std::fixed << std::setprecision(1) << (bps / (1024.0 * 1024 * 1024)) << L" GB/s";
    return oss.str();
}

std::wstring ConsoleRenderer::Truncate(const std::wstring& s, size_t max) {
    if (s.size() <= max) return s;
    if (max <= 3) return s.substr(0, max);
    return s.substr(0, max - 3) + L"...";
}

void ConsoleRenderer::Update(const ViewModel& view) {
    SHORT row = 0;
    WriteAt(0, row++, L"Volume: " + view.Volume);
    WriteAt(0, row++, L"Lost Events: " + std::to_wstring(view.Diags.LostEvents.load()));
    WriteAt(0, row++, L"Lost Buffers: " + std::to_wstring(view.Diags.LostBuffers.load()));
    row++;

    WriteAt(0, row++, L"PID     Process             R_IOPS  W_IOPS  Read/s     Write/s    Total
/s");
    WriteAt(0, row++, L"-----------------------------------------------------------------------
--");

    const size_t maxRows = 50;
    for (size_t i = 0; i < view.Rows.size() && i < maxRows; ++i) {
        const auto& r = view.Rows[i];
        std::wostringstream line;
        line << std::setw(8) << r.Identity.Pid << L" "
             << std::left << std::setw(20) << Truncate(r.ProcessName, 20) << L" "
             << std::setw(7) << r.ReadIOPS << L" "
             << std::setw(7) << r.WriteIOPS << L" "
             << std::setw(10) << FormatRate(r.ReadPerSecond) << L" "
             << std::setw(10) << FormatRate(r.WritePerSecond) << L" "
             << FormatRate(r.ReadPerSecond + r.WritePerSecond);
        WriteAt(0, static_cast<SHORT>(row++), line.str());
    }
    for (size_t i = view.Rows.size(); i < maxRows; ++i)
        WriteAt(0, static_cast<SHORT>(row++), std::wstring(80, L' '));

    row += 2;
    std::wostringstream footer;
    footer << L"Processes: " << view.Rows.size()
           << L" | DecodeErr: " << view.Diags.SchemaDecodeFailures.load()
           << L" | UnknownFO: " << view.Diags.UnknownFileObjects.load()
           << L" | Ev/s: " << view.EventsPerSec
           << L" | Session: " << (view.Owned ? L"Owned" : L"Attached");
    WriteAt(0, static_cast<SHORT>(row++), footer.str());
    WriteAt(0, static_cast<SHORT>(row++), L"Hotkeys: r Read/s  w Write/s  i IOPS  t Total/s  p
PID  q Quit");
}

void ConsoleRenderer::HandleInput() {
    INPUT_RECORD rec{};
    DWORD read = 0;
    if (PeekConsoleInputW(GetStdHandle(STD_INPUT_HANDLE), &rec, 1, &read) && read) {
        ReadConsoleInputW(GetStdHandle(STD_INPUT_HANDLE), &rec, 1, &read);
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
            wchar_t ch = rec.Event.KeyEvent.uChar.UnicodeChar;
            if (ch == L'q' || ch == L'Q') quit_ = true;
            else if (ch == L'r') sort_ = SortKey::Read;
            else if (ch == L'w') sort_ = SortKey::Write;
            else if (ch == L'i') sort_ = SortKey::IOPS;
            else if (ch == L't') sort_ = SortKey::Total;
            else if (ch == L'p') sort_ = SortKey::Pid;
        } else if (rec.EventType == WINDOW_BUFFER_SIZE_EVENT) {
            // Resize handled by next Update
        }
    }
}

