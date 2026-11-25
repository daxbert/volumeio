#pragma once
#include "Diagnostics.h"
#include <windows.h>
#include <evntrace.h>
#include <functional>
#include <atomic>
#include <thread>
#include <memory>

ULONG WINAPI BufferCallback(PEVENT_TRACE_LOGFILE logfile);

class EtwSession {
public:
    using EventCallback = std::function<void(PEVENT_RECORD)>;

    explicit EtwSession(DiagnosticCounters& diags);
    ~EtwSession();

    bool Start(const ULONG requiredFlags);
    void Stop();
    void Run(); // blocks until Stop
    void SetEventCallback(EventCallback cb);

    bool IsOwned() const { return owned_; }
    ULONG GetEnableFlags() const { return enableFlags_; }

private:
    static void WINAPI EventCallbackStatic(PEVENT_RECORD event);

    DiagnosticCounters& diags_;
    TRACEHANDLE sessionHandle_ = INVALID_PROCESSTRACE_HANDLE;
    TRACEHANDLE consumerHandle_ = INVALID_PROCESSTRACE_HANDLE;
    std::atomic<bool> running_{false};
    std::atomic<bool> owned_{false};
    ULONG enableFlags_ = 0;
    EventCallback callback_;
    std::thread etwThread_;
    std::unique_ptr<uint8_t[]> propsBuffer_;
};
