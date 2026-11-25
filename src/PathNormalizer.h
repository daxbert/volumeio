#pragma once
#include <string>

std::wstring NormalizePath(const std::wstring& path);
bool MatchesTargetVolume(const std::wstring& normalizedPath, const std::wstring& targetNtPath);
