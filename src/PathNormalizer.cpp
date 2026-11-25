#include "PathNormalizer.h"
#include <algorithm>
#include <cwctype>

std::wstring NormalizePath(const std::wstring& path) {
    if (path.empty()) return {};

    std::wstring result;
    result.reserve(path.size());

    // Uppercase + replace / with \
    for (wchar_t c : path) {
        if (c == L'/') result.push_back(L'\\');
        else result.push_back(static_cast<wchar_t>(towupper(c)));
    }

    // Collapse duplicate separators
    std::wstring collapsed;
    collapsed.reserve(result.size());
    bool lastSep = false;
    for (wchar_t c : result) {
        if (c == L'\\') {
            if (!lastSep) {
                collapsed.push_back(c);
                lastSep = true;
            }
        } else {
            collapsed.push_back(c);
            lastSep = false;
        }
    }

    // Strip trailing separator except for root forms
    // Root forms: ends with volume root (e.g. \DEVICE\HARDDISKVOLUME7\ or \\?\VOLUME{...}\ )
    if (collapsed.size() > 1 && collapsed.back() == L'\\') {
        bool isRoot = false;
        // Check for classic NT volume root or volume GUID root
        if (collapsed.rfind(L"\\DEVICE\\HARDDISKVOLUME", 0) == 0) {
            size_t last = collapsed.rfind(L'\\');
            if (last != std::wstring::npos && last > 0) {
                std::wstring without = collapsed.substr(0, last);
                if (without.find(L'\\', 1) == std::wstring::npos ||
                    without.rfind(L"\\DEVICE\\HARDDISKVOLUME") == 0 &&
                    without.find(L'\\', 1) == std::wstring::npos) {
                    // More precise: volume root has exactly one trailing \ after volume number
                    size_t volEnd = without.find_last_of(L'\\');
                    if (volEnd == std::wstring::npos || without.find(L'\\', volEnd + 1) == std::wstring::npos)
                        isRoot = true;
                }
            }
        } else if (collapsed.rfind(L"\\\\?\\VOLUME{", 0) == 0 && collapsed.find(L'}') != std::wstring::npos) {
            size_t brace = collapsed.find(L'}');
            if (brace != std::wstring::npos && brace + 1 < collapsed.size() && collapsed[brace + 1] == L'\\' &&
                collapsed.find(L'\\', brace + 2) == std::wstring::npos)
                isRoot = true;
        }
        if (!isRoot) {
            collapsed.pop_back();
        }
    }
    return collapsed;
}

bool MatchesTargetVolume(const std::wstring& normalizedPath, const std::wstring& targetNtPath) {
    if (normalizedPath.empty() || targetNtPath.empty()) return false;

    std::wstring path = NormalizePath(normalizedPath);
    std::wstring target = NormalizePath(targetNtPath);

    if (path.size() < target.size()) return false;
    if (path.compare(0, target.size(), target) != 0) return false;

    if (path.size() == target.size()) return true;
    return path[target.size()] == L'\\';
}
