# VolumeIO

Real-time per-process file I/O monitor for a specific Windows volume, built on the classic NT Kernel Logger (ETW).

`volumeio.exe M:` displays live read/write throughput and IOPS attributed to the processes generating the activity on the given drive.

## Features

- Classic NT Kernel Logger session (`KERNEL_LOGGER_NAME` + `SystemTraceControlGuid`)
- Ownership-aware: starts a new session or attaches to an existing one without modifying it
- FileObject correlation (`uint64_t`) with volume filtering performed only on Create/Name events
- Process identity as `(Pid, CreationTime)` — handles PID reuse and synthetic identities
- Real-time and cumulative views
- Partial console updates (no full-screen clears)
- Diagnostic counters (lost events, decode failures, unknown FileObjects, etc.)

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 (or Build Tools) with the C++ workload
- Administrator privileges (required for kernel ETW sessions)
- CMake 3.20+

## Build

From a **Developer Command Prompt for VS 2022**:

```cmd
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The executable is produced at `build\Release\volumeio.exe`.

### Unit tests

```cmd
cmake --build build --config Release --target volumeio_tests
build\Release\volumeio_tests.exe
```

## Usage

```cmd
volumeio.exe M:
```

- Drive letter must be in `X:` form and must exist.
- The process must be run elevated.

### Hotkeys

| Key | Action |
|-----|--------|
| `t` | Sort by Total/s (default) |
| `r` | Sort by Read/s |
| `w` | Sort by Write/s |
| `i` | Sort by IOPS |
| `p` | Sort by PID |
| `h` | Toggle visibility of terminated processes |
| `c` | Toggle real-time ↔ cumulative totals |
| `q` | Quit |

### Display modes

- **Real-time** (default): bytes/sec and IOPS over the last interval
- **Cumulative**: total bytes and operation counts since the process was first observed

Terminated processes are hidden by default (`h` to show them). The process count in the footer reflects only the rows currently displayed.

## Architecture

| Component | Responsibility |
|-----------|----------------|
| `EtwSession` | Session ownership state machine, StartTrace / attach / stop |
| `EventDispatch` | TDH decode and opcode routing |
| `FileObjectMap` | FileObject → path/volume cache, orphan reaping |
| `ProcessStats` | Per-process aggregation and name resolution |
| `RateCalculator` | 1-second QPC rate snapshot |
| `ConsoleRenderer` | Partial console updates, hotkeys, dynamic sizing |
| `PathNormalizer` / `VolumeMapper` | Path canonicalization and drive ↔ NT device mapping |

Only the following kernel events are consumed:  
`FileIo/Create`, `FileIo/Name`, `FileIo/Read`, `FileIo/Write`, `FileIo/Cleanup`, `FileIo/Close`, `Process/Start`, `Process/End`.

`EVENT_TRACE_FLAG_DISK_IO` is intentionally not enabled.

## Diagnostics

The footer reports:

- Lost events / lost buffers
- Decode errors (TDH / schema failures)
- Unknown FileObjects (Read/Write for a FileObject never seen in Create/Name)
- Events/sec
- Session mode (Owned / Attached)

Unknown FileObjects are expected when attaching to an already-running logger or when files were opened before the session started; those I/Os cannot be attributed to a volume.

## Notes

- The tool never modifies an NT Kernel Logger session it did not start.
- Path matching occurs only on Create and Name events; the result is cached for subsequent Read/Write events.
- Process names for short-lived processes may appear as `[pid]` if the process exits before name resolution succeeds.
- PID 4 is hard-coded as `System`.

## License

MIT 

## Authors

- Dax Eckenberg — design, integration, testing, and maintenance
- Grok (xAI) — implementation assistance and code generation
