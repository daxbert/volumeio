#include "TdhUtils.h"
#include <vector>

bool TdhUtils::GetEventInformation(PEVENT_RECORD event, std::unique_ptr<uint8_t[]>& buffer, PTRACE_EVENT_INFO& info, TDH_CONTEXT& context) {
    context = {};
    context.ParameterValue = sizeof(void*);
    context.ParameterType = TDH_CONTEXT_POINTERSIZE;

    ULONG bufferSize = 0;
    TDHSTATUS status = TdhGetEventInformation(event, 1, &context, nullptr, &bufferSize);
    if (status != ERROR_INSUFFICIENT_BUFFER) return false;

    buffer = std::make_unique<uint8_t[]>(bufferSize);
    info = reinterpret_cast<PTRACE_EVENT_INFO>(buffer.get());
    status = TdhGetEventInformation(event, 1, &context, info, &bufferSize);
    return status == ERROR_SUCCESS;
}

static PROPERTY_DATA_DESCRIPTOR MakeDesc(const wchar_t* name) {
    PROPERTY_DATA_DESCRIPTOR desc{};
    desc.PropertyName = reinterpret_cast<ULONGLONG>(name);
    desc.ArrayIndex = ULONG_MAX;
    return desc;
}

std::optional<uint32_t> TdhUtils::GetPropertyUInt32(PEVENT_RECORD event, PTRACE_EVENT_INFO info, const wchar_t* name, TDH_CONTEXT* context) {
    PROPERTY_DATA_DESCRIPTOR desc = MakeDesc(name);
    ULONG size = 0;
    TDHSTATUS status = TdhGetPropertySize(event, 1, context, 1, &desc, &size);
    if (status != ERROR_SUCCESS || size < sizeof(uint32_t)) return std::nullopt;

    uint32_t value = 0;
    status = TdhGetProperty(event, 1, context, 1, &desc, sizeof(value), reinterpret_cast<PBYTE>(&value));
    if (status != ERROR_SUCCESS) return std::nullopt;
    return value;
}

std::optional<uint64_t> TdhUtils::GetPropertyUInt64(PEVENT_RECORD event, PTRACE_EVENT_INFO info, const wchar_t* name, TDH_CONTEXT* context) {
    PROPERTY_DATA_DESCRIPTOR desc = MakeDesc(name);
    ULONG size = 0;
    TDHSTATUS status = TdhGetPropertySize(event, 1, context, 1, &desc, &size);
    if (status != ERROR_SUCCESS || size < sizeof(uint64_t)) return std::nullopt;

    uint64_t value = 0;
    status = TdhGetProperty(event, 1, context, 1, &desc, sizeof(value), reinterpret_cast<PBYTE>(&value));
    if (status != ERROR_SUCCESS) return std::nullopt;
    return value;
}

std::optional<std::wstring> TdhUtils::GetPropertyString(PEVENT_RECORD event, PTRACE_EVENT_INFO info, const wchar_t* name, TDH_CONTEXT* context) {
    PROPERTY_DATA_DESCRIPTOR desc = MakeDesc(name);
    ULONG size = 0;
    TDHSTATUS status = TdhGetPropertySize(event, 1, context, 1, &desc, &size);
    if (status != ERROR_SUCCESS || size == 0)
        return std::nullopt;

    std::vector<BYTE> buf(size + sizeof(wchar_t), 0);  // extra null safety
    status = TdhGetProperty(event, 1, context, 1, &desc, size, buf.data());
    if (status != ERROR_SUCCESS)
        return std::nullopt;

    // Prefer wide interpretation, but stop at the first null
    const wchar_t* wide = reinterpret_cast<const wchar_t*>(buf.data());
    size_t maxChars = size / sizeof(wchar_t);
    size_t len = 0;
    while (len < maxChars && wide[len] != L'\0')
        ++len;

    if (len > 0)
        return std::wstring(wide, len);

    // Fallback: treat as ANSI if the wide view was empty/garbage
    const char* ansi = reinterpret_cast<const char*>(buf.data());
    size_t ansiLen = 0;
    while (ansiLen < size && ansi[ansiLen] != '\0')
        ++ansiLen;
    if (ansiLen > 0) {
        int needed = MultiByteToWideChar(CP_ACP, 0, ansi, static_cast<int>(ansiLen), nullptr, 0);
        if (needed > 0) {
            std::wstring result(needed, L'\0');
            MultiByteToWideChar(CP_ACP, 0, ansi, static_cast<int>(ansiLen), result.data(), needed);
            return result;
        }
    }
    return std::nullopt;
}
