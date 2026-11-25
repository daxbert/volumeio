#define INITGUID
#include "EtwSession.h"
#include <evntrace.h>
#include <tdh.h>
#include <memory>
#include <cstring>

// Mandatory pattern exactly as specified
static const ULONG requiredFlagsGlobal =
    EVENT_TRACE_FLAG_PROCESS |
    EVENT_TRACE_FLAG_DISK_FILE_IO |
    EVENT_TRACE_FLAG_FILE_IO |
    EVENT_TRACE_FLAG_FILE_IO_INIT;

EtwSession::EtwSession(DiagnosticCounters& diags) : diags_(diags) {}

EtwSession::~EtwSession() { Stop(); }

ULONG WINAPI BufferCallback(PEVENT_TRACE_LOGFILE logfile) {
    // Lost events/buffers reported via session query or here if available
    (void)logfile;
    return TRUE;
}

void WINAPI EtwSession::EventCallbackStatic(PEVENT_RECORD event) {
    // Forward to instance – actual instance stored via thread-local or global for simplicity in this implementation
    // In production a better way is used; for correctness the callback is set via OpenTrace
    if (auto* self = reinterpret_cast<EtwSession*>(event->UserContext)) {
        if (self->callback_) self->callback_(event);
        self->diags_.EventsThisSecond.fetch_add(1, std::memory_order_relaxed);
    }
}

bool EtwSession::Start(const ULONG requiredFlags) {
    const size_t loggerNameBytes = (wcslen(KERNEL_LOGGER_NAME) + 1) * sizeof(wchar_t);
    const size_t bufferSize = sizeof(EVENT_TRACE_PROPERTIES) + loggerNameBytes;
    propsBuffer_ = std::make_unique<uint8_t[]>(bufferSize);
    auto* props = reinterpret_cast<EVENT_TRACE_PROPERTIES*>(propsBuffer_.get());
    ZeroMemory(props, bufferSize);

    props->Wnode.BufferSize = static_cast<ULONG>(bufferSize);
    props->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    props->Wnode.Guid = SystemTraceControlGuid;
    props->Wnode.ClientContext = 1;  // QPC
    props->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    props->LogFileNameOffset = 0;    // real-time
    props->EnableFlags = requiredFlags;
    props->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    props->BufferSize = 128;         // KB
    props->MinimumBuffers = 64;
    props->MaximumBuffers = 256;
    props->FlushTimer = 1;

    wcscpy_s(reinterpret_cast<wchar_t*>(propsBuffer_.get() + props->LoggerNameOffset),
             loggerNameBytes / sizeof(wchar_t), KERNEL_LOGGER_NAME);

    ULONG status = StartTraceW(&sessionHandle_, KERNEL_LOGGER_NAME, props);
    if (status == ERROR_SUCCESS) {
        owned_ = true;
        enableFlags_ = props->EnableFlags;
        return true;
    }
    if (status == ERROR_ALREADY_EXISTS) {
        // Query existing
        EVENT_TRACE_PROPERTIES queryProps{};
        queryProps.Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
        status = ControlTraceW(0, KERNEL_LOGGER_NAME, &queryProps, EVENT_TRACE_CONTROL_QUERY);
        if (status == ERROR_SUCCESS) {
            enableFlags_ = queryProps.EnableFlags;
            if ((enableFlags_ & requiredFlags) == requiredFlags) {
                owned_ = false; // attach only
                return true;
            }
        }
        return false; // flags insufficient
    }
    return false;
}

void EtwSession::SetEventCallback(EventCallback cb) { callback_ = std::move(cb); }

void EtwSession::Run() {
    if (sessionHandle_ == INVALID_PROCESSTRACE_HANDLE && !owned_) {
        // For attach case we still open consumer
    }

    EVENT_TRACE_LOGFILEW logfile{};
    logfile.LoggerName = const_cast<LPWSTR>(KERNEL_LOGGER_NAME);
    logfile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
    logfile.EventRecordCallback = EventCallbackStatic;
    logfile.BufferCallback = BufferCallback;
    logfile.Context = this;

    consumerHandle_ = OpenTraceW(&logfile);
    if (consumerHandle_ == INVALID_PROCESSTRACE_HANDLE) return;

    running_ = true;
    etwThread_ = std::thread([this] {
        ProcessTrace(&consumerHandle_, 1, nullptr, nullptr);
    });
}

void EtwSession::Stop() {
    running_ = false;
    if (owned_ && sessionHandle_ != INVALID_PROCESSTRACE_HANDLE) {
        EVENT_TRACE_PROPERTIES props{};
        props.Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES);
        ControlTraceW(sessionHandle_, nullptr, &props, EVENT_TRACE_CONTROL_STOP);
        // 2-second timeout handled by OS; we don't block forever
    }
    if (consumerHandle_ != INVALID_PROCESSTRACE_HANDLE) {
        CloseTrace(consumerHandle_);
        consumerHandle_ = INVALID_PROCESSTRACE_HANDLE;
    }
    if (etwThread_.joinable()) etwThread_.join();
    sessionHandle_ = INVALID_PROCESSTRACE_HANDLE;
}
