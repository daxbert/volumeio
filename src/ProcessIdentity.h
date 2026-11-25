#pragma once
#include <windows.h>
#include <cstdint>
#include <map>

struct ProcessIdentity {
    DWORD Pid = 0;
    FILETIME CreationTime{};

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


