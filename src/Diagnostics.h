#pragma once
#include <cstdint>
#include <atomic>

struct DiagnosticCounters {
    std::atomic<uint64_t> LostEvents{0};
    std::atomic<uint64_t> LostBuffers{0};
    std::atomic<uint64_t> SchemaDecodeFailures{0};
    std::atomic<uint64_t> UnknownEventVersions{0};
    std::atomic<uint64_t> PointerSizeMismatches{0};
    std::atomic<uint64_t> UnknownFileObjects{0};
    std::atomic<uint64_t> DroppedEvents{0};
    std::atomic<uint64_t> NameResolutionFailures{0};
    std::atomic<uint64_t> OrphanCleanupEvents{0};
    std::atomic<uint64_t> OrphanCloseEvents{0};
    std::atomic<uint64_t> VolumeMatchFailures{0};
    std::atomic<uint64_t> EventsThisSecond{0};

    DiagnosticCounters() = default;
    DiagnosticCounters(const DiagnosticCounters&) = delete;
    DiagnosticCounters& operator=(const DiagnosticCounters&) = delete;

    void ResetSecond() { EventsThisSecond.store(0, std::memory_order_relaxed); }
};
