#pragma once
#include <windows.h>
#include <evntrace.h>
#include <tdh.h>
#include <optional>
#include <string>
#include <memory>

class TdhUtils {
public:
    static std::optional<uint32_t> GetPropertyUInt32(PEVENT_RECORD event, PTRACE_EVENT_INFO info, const wchar_t* name, TDH_CONTEXT* context);
    static std::optional<uint64_t> GetPropertyUInt64(PEVENT_RECORD event, PTRACE_EVENT_INFO info, const wchar_t* name, TDH_CONTEXT* context);
    static std::optional<std::wstring> GetPropertyString(PEVENT_RECORD event, PTRACE_EVENT_INFO info, const wchar_t* name, TDH_CONTEXT* context);
    static bool GetEventInformation(PEVENT_RECORD event, std::unique_ptr<uint8_t[]>& buffer, PTRACE_EVENT_INFO& info, TDH_CONTEXT& context);
};
