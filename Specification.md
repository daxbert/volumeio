# VolumeIO - Native Windows ETW Volume Activity Monitor
## Master Project Specification and Execution Plan

---

# Purpose

Create a production-quality Windows 11 x64 command-line utility:

```cmd
volumeio.exe M:
```

that displays live read/write activity occurring on a specific volume.

Example:

```text
Volume: M:

PID     Process              Read/s       Write/s
--------------------------------------------------
4248    ffmpeg.exe           2.2 GB/s     0
552     System              120 MB/s      0
9328    python.exe           0          850 MB/s
```

The tool shall identify only I/O occurring on the specified volume.

---

# Technical Requirements

## Language

C++20

## Platform

Windows 11 x64

## Compiler

Visual Studio 2022

MSVC

## Runtime Dependencies

None

The final product must be:

- Single executable
- No .NET
- No Python
- No Boost
- No Qt
- No ncurses
- No third-party ETW wrappers
- No external DLLs beyond normal Windows system DLLs

## Allowed Libraries

Windows SDK only.

Including:

```cpp
windows.h
evntrace.h
evntcons.h
tdh.h
tlhelp32.h
processthreadsapi.h
```

## Link Libraries

```text
advapi32.lib
tdh.lib
```

---

# Functional Requirements

The program shall:

1. Monitor realtime ETW File I/O activity.
2. Allow filtering by drive letter.
3. Correlate FileObjects to filenames.
4. Resolve NT device paths.
5. Aggregate read activity per process.
6. Aggregate write activity per process.
7. Resolve process names.
8. Present live console UI.
9. Operate continuously until exit.
10. Support hot refresh at 1 second intervals.

---

# Architecture

The system shall consist of the following modules.

```text
src/

  main.cpp

  common/
      Types.h
      Types.cpp

  volume/
      VolumeMapper.h
      VolumeMapper.cpp

  etw/
      KernelSession.h
      KernelSession.cpp

      EventDecoder.h
      EventDecoder.cpp

      EventSchemas.h
      EventSchemas.cpp

  filemap/
      FileObjectTracker.h
      FileObjectTracker.cpp

  stats/
      ProcessAggregator.h
      ProcessAggregator.cpp

  filter/
      VolumeFilter.h
      VolumeFilter.cpp

  ui/
      ConsoleRenderer.h
      ConsoleRenderer.cpp
```

---

# Phase 1
# Volume Mapper

## Objective

Create mapping between DOS drive letters and kernel device paths.

Example:

```text
M:
    ->
\Device\HarddiskVolume7
```

And reverse mapping:

```text
\Device\HarddiskVolume7
    ->
M:
```

## APIs

```cpp
QueryDosDeviceW
GetLogicalDriveStringsW
```

## Deliverables

```text
VolumeMapper.h
VolumeMapper.cpp
```

## Generation Prompt

Generate VolumeMapper.h and VolumeMapper.cpp.

Requirements:

- C++20
- Windows SDK only
- No third party libraries

Implement:

class VolumeMapper

Methods:

    EnumerateVolumes()
    DriveToDevice()
    DeviceToDrive()

Maintain bidirectional mappings:

    M: -> \Device\HarddiskVolume7

and

    \Device\HarddiskVolume7 -> M:

Thread-safe.

Return complete compilable source.

---

# Phase 2
# Kernel ETW Session Manager

## Objective

Create and control realtime kernel ETW session.

## Capabilities

Start:

```cpp
StartTrace()
```

Enable providers.

Open:

```cpp
OpenTrace()
```

Process:

```cpp
ProcessTrace()
```

Stop:

```cpp
ControlTrace()
```

## Required Events

Enable:

```text
FILE_IO
DISK_IO
PROCESS
```

## Deliverables

```text
KernelSession.h
KernelSession.cpp
```

## Generation Prompt

Generate KernelSession.h and KernelSession.cpp.

Requirements:

- Native C++20
- Windows ETW APIs only

Features:

- Create realtime kernel session
- Enable FileIO
- Enable DiskIO
- Enable Process events

Expose callback registration:

    std::function<void(EVENT_RECORD*)>

Support:

Start()
Stop()
Run()

Handle cleanup correctly.

Provide complete compilable implementation.

---

# Phase 3
# Generic TDH Event Decoder

## Objective

Provide generic property extraction.

## APIs

```cpp
TdhGetEventInformation
TdhGetProperty
```

## Deliverables

```text
EventDecoder.h
EventDecoder.cpp
```

## Generation Prompt

Generate EventDecoder.h and EventDecoder.cpp.

Requirements:

Use:

    TdhGetEventInformation
    TdhGetProperty

Implement:

GetPropertyUInt32()
GetPropertyUInt64()
GetPropertyString()

Return std::optional values.

Support:

- UINT32
- UINT64
- Unicode strings
- Binary blobs

Include helper methods for property discovery.

Return complete compilable implementation.

---

# Phase 4
# FileObject Correlation Engine

## Objective

Maintain FileObject -> Filename correlation.

This is required because many FileIO Read and Write events only expose:

```text
FileObject
```

rather than filename.

## Data Structure

```cpp
uint64_t FileObject
    ->
std::wstring Path
```

## Events Consumed

```text
FileIo/Create
FileIo/Cleanup
FileIo/Close
```

## Deliverables

```text
FileObjectTracker.h
FileObjectTracker.cpp
```

## Generation Prompt

Generate FileObjectTracker.h and FileObjectTracker.cpp.

Requirements:

Maintain:

    uint64_t FileObject
            ->
    std::wstring Path

Operations:

    OnCreate(...)
    OnCleanup(...)
    OnClose(...)

    Lookup(...)

Use:

    std::shared_mutex

Thread-safe.

Automatically remove stale entries.

Return complete compilable source.

---

# Phase 5
# Kernel Event Schema Layer

## Objective

Convert raw ETW events into strongly typed structures.

## Schemas Required

### Create

```cpp
FileCreateEvent
```

### Cleanup

```cpp
FileCleanupEvent
```

### Close

```cpp
FileCloseEvent
```

### Read

```cpp
FileReadEvent
```

### Write

```cpp
FileWriteEvent
```

## Fields

```text
ProcessId
FileObject
IoSize
FileName
Timestamp
```

## Deliverables

```text
EventSchemas.h
EventSchemas.cpp
```

## Generation Prompt

Generate EventSchemas.h and EventSchemas.cpp.

Target:

Windows Kernel FileIO provider.

Implement strongly typed structures:

    FileCreateEvent
    FileCleanupEvent
    FileCloseEvent
    FileReadEvent
    FileWriteEvent

Provide parser functions using EventDecoder.

Return complete compilable implementation.

---

# Phase 6
# Process Statistics Aggregator

## Objective

Track read and write totals by process.

## Data Maintained

```cpp
PID
ProcessName
ReadBytes
WriteBytes
```

## APIs

```cpp
OpenProcess
QueryFullProcessImageNameW
```

## Deliverables

```text
ProcessAggregator.h
ProcessAggregator.cpp
```

## Generation Prompt

Generate ProcessAggregator.h and ProcessAggregator.cpp.

Requirements:

Track per-process:

    ReadBytes
    WriteBytes

Methods:

    AddRead()
    AddWrite()

    SnapshotAndReset()

Resolve process names.

Must be thread-safe.

Prefer atomics.

Provide complete compilable implementation.

---

# Phase 7
# Volume Filtering Layer

## Objective

Determine if activity belongs to user-selected volume.

## Inputs

Possible path formats:

```text
M:\Movie.mkv

\Device\HarddiskVolume7\Movie.mkv
```

## Deliverables

```text
VolumeFilter.h
VolumeFilter.cpp
```

## Generation Prompt

Generate VolumeFilter.h and VolumeFilter.cpp.

Requirements:

Support DOS paths.

Support NT device paths.

Use VolumeMapper.

Implement:

    MatchesVolume(std::wstring)

Return complete compilable implementation.

---

# Phase 8
# Console Renderer

## Objective

Provide live terminal UI.

## Columns

```text
PID
Process
Read/s
Write/s
```

## Features

Keys:

```text
r = sort by reads
w = sort by writes
t = sort by total
q = quit
```

Refresh:

```text
1 second
```

Avoid full-screen flicker.

Use Win32 console APIs.

No third-party UI libraries.

## Deliverables

```text
ConsoleRenderer.h
ConsoleRenderer.cpp
```

## Generation Prompt

Generate ConsoleRenderer.h and ConsoleRenderer.cpp.

Requirements:

Pure Win32 console.

Features:

- Refresh every second
- Sortable columns
- Hotkeys:
    r
    w
    t
    q

Avoid screen flicker.

Provide complete compilable implementation.

---

# Phase 9
# System Integration

## Objective

Assemble the final executable.

## Workflow

Startup:

```text
Resolve volume
Start ETW session
Initialize trackers
Initialize renderer
```

Realtime:

```text
Create Event
 ->
Update FileObject Map

Read Event
 ->
Resolve Filename

Filename
 ->
Volume Filter

Match
 ->
Aggregate Statistics

Statistics
 ->
UI Refresh
```

Shutdown:

```text
Stop ETW
Dispose handles
Exit cleanly
```

## Deliverables

```text
main.cpp
```

## Generation Prompt

Generate main.cpp.

Integrate:

VolumeMapper
KernelSession
EventDecoder
EventSchemas
FileObjectTracker
ProcessAggregator
VolumeFilter
ConsoleRenderer

Implement complete application:

volumeio.exe M:

Return complete compilable source.

---

# Phase 10
# Build System

## Deliverables

```text
VolumeIO.sln
VolumeIO.vcxproj
```

## Generation Prompt

Generate Visual Studio 2022 project files.

Requirements:

- C++20
- x64
- Release
- Unicode

Link:

    advapi32.lib
    tdh.lib

Include all source files created in prior phases.

Provide complete XML for:

    .vcxproj
    .filters

---

# Validation Phase

## Test 1

Large file creation.

```cmd
fsutil file createnew M:\test.bin 4294967296
```

Expected:

```text
System process writes visible.
```

---

## Test 2

Robocopy.

```cmd
robocopy M:\ N:\ /MT:32
```

Expected:

```text
Correct attribution.
```

---

## Test 3

FFmpeg Read Workload.

```cmd
ffmpeg -i M:\input.mkv -f null -
```

Expected:

```text
ffmpeg.exe reported.
```

---

## Test 4

Explorer Copy.

```text
Drag and drop large file.
```

Expected:

```text
explorer.exe reported.
```

---

# Final Audit Prompt

After all code is generated and integrated:

Review the entire codebase.

Analyze for:

- ETW session leaks
- HANDLE leaks
- Memory leaks
- Race conditions
- shared_mutex misuse
- TDH buffer sizing errors
- Unicode issues
- Device-path conversion bugs
- Lost-event handling
- Cleanup correctness

Do not rewrite architecture.

Provide only patches and improvements.

Output unified diffs.

---

# Success Criteria

The project is considered complete when:

1. volumeio.exe builds cleanly.
2. Runs on Windows 11 x64.
3. Requires only Administrator privileges.
4. Correctly correlates FileObject-driven reads and writes.
5. Correctly filters by target drive letter.
6. Correctly attributes I/O to processes.
7. Produces live updates every second.
8. Requires no external runtime or libraries.
9. Consists of a single distributable executable.
