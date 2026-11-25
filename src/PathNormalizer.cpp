#include "PathNormalizer.h"
#include <algorithm>
#include <cwctype>

std::wstring NormalizePath(const std::wstring& path) {
    if (path.empty())
        return {};

    std::wstring result;
    result.reserve(path.size());

    for (wchar_t ch : path) {
        if (ch == L'/')
            result.push_back(L'\\');
        else
            result.push_back(static_cast<wchar_t>(towupper(ch)));
    }

    std::wstring collapsed;
    collapsed.reserve(result.size());
    bool lastWasSep = false;
    for (wchar_t ch : result) {
        if (ch == L'\\') {
            if (!lastWasSep) {
                collapsed.push_back(ch);
                lastWasSep = true;
            }
        } else {
            collapsed.push_back(ch);
            lastWasSep = false;
        }
    }

    // Preserve trailing separator only for volume roots
    if (collapsed.size() > 1 && collapsed.back() == L'\\') {
        bool preserve = false;

        if (collapsed.rfind(L"\\DEVICE\\HARDDISKVOLUME", 0) == 0) {
            size_t last = collapsed.rfind(L'\\');
            if (last != std::wstring::npos) {
                std::wstring without = collapsed.substr(0, last);
                if (without.find(L'\\', 1) == std::wstring::npos)
                    preserve = true;
            }
        } else if (collapsed.rfind(L"\\\\?\\VOLUME{", 0) == 0) {
            size_t brace = collapsed.find(L'}');
            if (brace != std::wstring::npos &&
                brace + 1 < collapsed.size() &&
                collapsed[brace + 1] == L'\\' &&
                collapsed.find(L'\\', brace + 2) == std::wstring::npos)
                preserve = true;
        }

        if (!preserve)
            collapsed.pop_back();
    }
    return collapsed;
}

bool MatchesTargetVolume(const std::wstring& normalizedPath, const std::wstring& targetNtPath) {
    if (normalizedPath.empty() || targetNtPath.empty())
        return false;

    std::wstring path = NormalizePath(normalizedPath);
    std::wstring target = NormalizePath(targetNtPath);

    if (path.size() < target.size())
        return false;
    if (path.compare(0, target.size(), target) != 0)
        return false;

    return path.size() == target.size() || path[target.size()] == L'\\';
}


