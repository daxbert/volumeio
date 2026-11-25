#include "EventDispatch.h"

EventDispatch::EventDispatch(FileObjectMap& files, ProcessStats& stats, DiagnosticCounters& diags, const std::wstring& targetNt)
    : files_(files), stats_(stats), diags_(diags), targetNt_(targetNt) {}

void EventDispatch::OnEvent(PEVENT_RECORD event) {
    if (!event) return;

    TDH_CONTEXT context{};
    std::unique_ptr<uint8_t[]> buffer;
    PTRACE_EVENT_INFO info = nullptr;
    if (!TdhUtils::GetEventInformation(event, buffer, info, context)) {
        diags_.SchemaDecodeFailures.fetch_add(1, std::memory_order_relaxed);
        diags_.DroppedEvents.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    UCHAR opcode = event->EventHeader.EventDescriptor.Opcode;
    DWORD pid = event->EventHeader.ProcessId;

    switch (opcode) {
    case 64: HandleCreate(event, info, context); break;
    case 0:
    case 32: HandleName(event, info, context); break;
    case 67: HandleReadWrite(event, info, context, false); break;
    case 68: HandleReadWrite(event, info, context, true); break;
    case 65: HandleCleanup(event, info, context); break;
    case 66: HandleClose(event, info, context); break;
    case 1: HandleProcessStart(event, info, context); break;
    case 2: HandleProcessEnd(event); break;
    default: break;
    }
}

void EventDispatch::HandleCreate(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx) {
    auto fo = TdhUtils::GetPropertyUInt64(event, info, L"FileObject", &ctx);
    auto path = TdhUtils::GetPropertyString(event, info, L"OpenPath", &ctx);
    if (!fo || !path) {
        diags_.SchemaDecodeFailures.fetch_add(1, std::memory_order_relaxed);
        diags_.DroppedEvents.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    files_.OnCreate(*fo, *path);
}

void EventDispatch::HandleName(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx) {
    auto fo = TdhUtils::GetPropertyUInt64(event, info, L"FileObject", &ctx);
    auto name = TdhUtils::GetPropertyString(event, info, L"FileName", &ctx);
    if (!fo || !name) {
        diags_.SchemaDecodeFailures.fetch_add(1, std::memory_order_relaxed);
        diags_.DroppedEvents.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    files_.OnName(*fo, *name);
}

void EventDispatch::HandleReadWrite(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx, bool isWrite) {
    auto fo = TdhUtils::GetPropertyUInt64(event, info, L"FileObject", &ctx);
    auto size = TdhUtils::GetPropertyUInt32(event, info, L"IoSize", &ctx);
    if (!fo || !size) {
        diags_.SchemaDecodeFailures.fetch_add(1, std::memory_order_relaxed);
        diags_.DroppedEvents.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    DWORD pid = event->EventHeader.ProcessId;
    FILETIME ct{}; // synthetic or from process map; simplified
    ProcessIdentity id{pid, ct};
    if (files_.OnReadWrite(*fo, *size, isWrite, pid, ct)) {
        stats_.AddIO(id, *size, isWrite);
    }
}

void EventDispatch::HandleCleanup(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx) {
    auto fo = TdhUtils::GetPropertyUInt64(event, info, L"FileObject", &ctx);
    if (!fo) {
        diags_.SchemaDecodeFailures.fetch_add(1, std::memory_order_relaxed);
        diags_.DroppedEvents.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    files_.OnCleanup(*fo);
}

void EventDispatch::HandleClose(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx) {
    auto fo = TdhUtils::GetPropertyUInt64(event, info, L"FileObject", &ctx);
    if (!fo) {
        diags_.SchemaDecodeFailures.fetch_add(1, std::memory_order_relaxed);
        diags_.DroppedEvents.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    files_.OnClose(*fo);
}

void EventDispatch::HandleProcessStart(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx) {
    auto image = TdhUtils::GetPropertyString(event, info, L"ImageFileName", &ctx);
    // CreateTime handling simplified; prefer header ProcessId
    DWORD pid = event->EventHeader.ProcessId;
    FILETIME ct{};
    // In full implementation extract CreateTime property
    stats_.OnProcessStart(pid, image.value_or(L""), ct);
}

void EventDispatch::HandleProcessEnd(PEVENT_RECORD event) {
    DWORD pid = event->EventHeader.ProcessId;
    stats_.OnProcessEnd(pid);
}
