cmake_minimum_required(VERSION 3.20)
project(VolumeIO LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive- /Zc:__cplusplus)
    add_compile_definitions(UNICODE _UNICODE)
endif()

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

# Main executable
add_executable(volumeio
    src/PathNormalizer.cpp
    src/VolumeMapper.cpp
    src/TdhUtils.cpp
    src/FileObjectMap.cpp
    src/ProcessStats.cpp
    src/EtwSession.cpp
    src/EventDispatch.cpp
    src/ConsoleRenderer.cpp
    src/RateCalculator.cpp
    src/main.cpp
)
target_include_directories(volumeio PRIVATE src)
target_link_libraries(volumeio PRIVATE advapi32 tdh)

# Unit tests
add_executable(volumeio_tests
    tests/test_PathNormalizer.cpp
    tests/test_VolumeMapper.cpp
    tests/test_ProcessIdentity.cpp
    tests/test_FileObjectMap.cpp
    tests/test_ProcessStats.cpp
    tests/test_RateCalculator.cpp
    src/PathNormalizer.cpp
    src/VolumeMapper.cpp
    src/FileObjectMap.cpp
    src/ProcessStats.cpp
    src/RateCalculator.cpp
)
target_include_directories(volumeio_tests PRIVATE src)
message(STATUS "Catch2WithMain location: ${Catch2_SOURCE_DIR}")
target_link_libraries(volumeio_tests PRIVATE 
    Catch2::Catch2 
    Catch2::Catch2WithMain
)
if(MSVC)
    target_link_options(volumeio_tests PRIVATE "/WHOLEARCHIVE:Catch2Main.lib")
endif()

# Generate version header from git
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
else()
    set(GIT_VERSION "unknown")
    set(GIT_COMMIT "unknown")
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/src/Version.h.in
    ${CMAKE_BINARY_DIR}/generated/Version.h
    @ONLY
)

target_include_directories(volumeio PRIVATE ${CMAKE_BINARY_DIR}/generated)

