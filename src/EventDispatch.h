#pragma once
#include "FileObjectMap.h"
#include "ProcessStats.h"
#include "TdhUtils.h"
#include "Diagnostics.h"
#include <windows.h>
#include <evntrace.h>

class EventDispatch {
public:
    EventDispatch(FileObjectMap& files, ProcessStats& stats, DiagnosticCounters& diags, const std::wstring& targetNt);

    void OnEvent(PEVENT_RECORD event);

private:
    void HandleCreate(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx);
    void HandleName(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx);
    void HandleReadWrite(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx, bool isWrite);
    void HandleCleanup(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx);
    void HandleClose(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx);
    void HandleProcessStart(PEVENT_RECORD event, PTRACE_EVENT_INFO info, TDH_CONTEXT& ctx);
    void HandleProcessEnd(PEVENT_RECORD event);

    FileObjectMap& files_;
    ProcessStats& stats_;
    DiagnosticCounters& diags_;
    std::wstring targetNt_;
};
