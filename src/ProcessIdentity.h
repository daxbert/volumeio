#pragma once
#include <windows.h>
#include <cstdint>

struct ProcessIdentity {
    DWORD Pid = 0;
    FILETIME CreationTime{};  // Zero if unavailable (synthetic identity)

    bool operator==(const ProcessIdentity& other) const {
        return Pid == other.Pid &&
               CreationTime.dwLowDateTime == other.CreationTime.dwLowDateTime &&
               CreationTime.dwHighDateTime == other.CreationTime.dwHighDateTime;
    }
    bool operator<(const ProcessIdentity& other) const {
        if (Pid != other.Pid) return Pid < other.Pid;
        if (CreationTime.dwHighDateTime != other.CreationTime.dwHighDateTime)
            return CreationTime.dwHighDateTime < other.CreationTime.dwHighDateTime;
        return CreationTime.dwLowDateTime < other.CreationTime.dwLowDateTime;
    }
};

namespace std {
template <>
struct hash<ProcessIdentity> {
    size_t operator()(const ProcessIdentity& id) const noexcept {
        size_t h1 = std::hash<DWORD>{}(id.Pid);
        size_t h2 = std::hash<DWORD>{}(id.CreationTime.dwLowDateTime);
        size_t h3 = std::hash<DWORD>{}(id.CreationTime.dwHighDateTime);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
}
