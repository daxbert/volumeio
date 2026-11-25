#include "PathNormalizer.h"
#include <algorithm>
#include <cwctype>

std::wstring NormalizePath(const std::wstring& path) {
    if (path.empty()) return {};

    std::wstring result;
    result.reserve(path.size());

    // Uppercase + replace / with backslash
    for (wchar_t ch : path) {
        if (ch == L'/')
            result.push_back(L'\\');
        else
            result.push_back(static_cast<wchar_t>(towupper(ch)));
    }

    // Collapse duplicate separators
    std::wstring collapsed;
    collapsed.reserve(result.size());
    bool lastSep = false;
    for (wchar_t ch : result) {
        if (ch == L'\\') {
            if (!lastSep) {
                collapsed.push_back(ch);
                lastSep = true;
            }
        } else {
            collapsed.push_back(ch);
            lastSep = false;
        }
    }

    // Strip trailing separator except for volume roots
    if (collapsed.size() > 1 && collapsed.back() == L'\\') {
        bool isRoot = false;

        // Classic NT volume root: \DEVICE\HARDDISKVOLUMEn\ 
        if (collapsed.rfind(L"\\DEVICE\\HARDDISKVOLUME", 0) == 0) {
            size_t lastSep = collapsed.rfind(L'\\');
            if (lastSep != std::wstring::npos) {
                std::wstring withoutTrailing = collapsed.substr(0, lastSep);
                // Volume root has no additional path components after the volume name
                if (withoutTrailing.find(L'\\', 1) == std::wstring::npos ||
                    withoutTrailing.rfind(L"\\DEVICE\\HARDDISKVOLUME") == 0) {
                    size_t volPos = withoutTrailing.find(L'\\', 1);
                    if (volPos == std::wstring::npos)
                        isRoot = true;
                }
            }
        }
        // Volume GUID root: \\?\VOLUME{...}\
        else if (collapsed.rfind(L"\\\\?\\VOLUME{", 0) == 0) {
            size_t brace = collapsed.find(L'}');
            if (brace != std::wstring::npos &&
                brace + 1 < collapsed.size() &&
                collapsed[brace + 1] == L'\\' &&
                collapsed.find(L'\\', brace + 2) == std::wstring::npos) {
                isRoot = true;
            }
        }

        if (!isRoot)
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

    if (path.size() == target.size())
        return true;
    return path[target.size()] == L'\\';
}


