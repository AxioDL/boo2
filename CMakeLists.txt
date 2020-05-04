cmake_minimum_required(VERSION 3.10 FATAL_ERROR) # because of c++17

if(BOO2_NX)
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/nx.cmake")
endif()

project(boo2 LANGUAGES C CXX VERSION 0.1)

if(NOT MSVC)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(NOT TARGET logvisor)
  add_subdirectory(logvisor)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
if(NOT TARGET hsh)
  find_package(hsh REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
endif()

add_subdirectory(lib/audiodev/soxr/src)

set(boo2_SRCS
  lib/ApplicationCommon.cpp
  lib/HshImplementation.cpp
  lib/WindowDecorations.cpp
  lib/WindowDecorationsRes.cpp
  lib/audiodev/AudioSubmix.cpp
  lib/audiodev/AudioVoice.cpp
  lib/audiodev/AudioVoiceEngine.cpp
  lib/audiodev/LtRtProcessing.cpp
  lib/audiodev/MIDICommon.cpp
  lib/audiodev/MIDIDecoder.cpp
  lib/audiodev/MIDIEncoder.cpp
  lib/audiodev/WAVOut.cpp
  lib/inputdev/DeviceBase.cpp
  lib/inputdev/CafeProPad.cpp
  lib/inputdev/RevolutionPad.cpp
  lib/inputdev/DolphinSmashAdapter.cpp
  lib/inputdev/NintendoPowerA.cpp
  lib/inputdev/DualshockPad.cpp
  lib/inputdev/GenericPad.cpp
  lib/inputdev/DeviceSignature.cpp
  lib/inputdev/DeviceFinder.cpp
  lib/inputdev/HIDParser.cpp
)
hsh_sources(lib/WindowDecorations.cpp)
set(boo2_LIBS "")
set(boo2_DEFS "")
set(boo2_INCS include)

if(WIN32)
elseif(APPLE)
elseif(NX)
  list(APPEND boo2_LIBS debug deko3dd optimized deko3d debug nxd optimized nx)
else()
  include(FindPkgConfig)
  pkg_check_modules(xcb IMPORTED_TARGET xcb xcb-icccm xcb-xkb xkbcommon-x11 xkbcommon)
  if(xcb_FOUND)
    list(APPEND boo2_LIBS PkgConfig::xcb)
    list(APPEND boo2_DEFS VK_USE_PLATFORM_XCB_KHR)
    message(STATUS "Enabling XCB")
  endif()
  pkg_check_modules(wayland IMPORTED_TARGET wayland-client wayland-egl wayland-scanner wayland-protocols xkbcommon)
  if(OFF AND wayland_FOUND)
    list(APPEND boo2_LIBS PkgConfig::wayland)
    list(APPEND boo2_DEFS VK_USE_PLATFORM_WAYLAND_KHR)
    pkg_get_variable(WAYLAND_SCANNER_COMMAND wayland-scanner wayland_scanner)
    pkg_get_variable(WAYLAND_REGISTRY_DIR wayland-protocols pkgdatadir)
    macro(add_wayland_extension name path)
      add_custom_command(OUTPUT ${name}-client-protocol.h ${name}-protocol.c
              COMMAND "${WAYLAND_SCANNER_COMMAND}" ARGS
              client-header "${WAYLAND_REGISTRY_DIR}/${path}" ${name}-client-protocol.h
              COMMAND "${WAYLAND_SCANNER_COMMAND}" ARGS
              private-code "${WAYLAND_REGISTRY_DIR}/${path}" ${name}-protocol.c
              COMMENT "Generating ${name} extension sources")
      list(APPEND boo2_SRCS ${name}-protocol.c)
    endmacro()
    add_wayland_extension(xdg-shell stable/xdg-shell/xdg-shell.xml)
    add_wayland_extension(xdg-decoration unstable/xdg-decoration/xdg-decoration-unstable-v1.xml)
    list(APPEND boo2_INCS "${CMAKE_CURRENT_BINARY_DIR}")
    message(STATUS "Enabling Wayland")
  endif()
  if(NOT xcb_FOUND AND NOT wayland_FOUND)
    message(FATAL_ERROR "Unable to locate development installations of XCB or Wayland")
  endif()
  pkg_check_modules(pulse IMPORTED_TARGET libpulse)
  if(NOT pulse_FOUND)
    message(FATAL_ERROR "Unable to locate development installation of libpulse")
  endif()
  list(APPEND boo2_SRCS lib/audiodev/PulseAudio.cpp)
  pkg_check_modules(udev IMPORTED_TARGET libudev)
  if(NOT udev_FOUND)
    message(FATAL_ERROR "Unable to locate development installation of libudev")
  endif()
  list(APPEND boo2_SRCS lib/inputdev/HIDListenerUdev.cpp lib/inputdev/HIDDeviceUdev.cpp)
  list(APPEND boo2_LIBS PkgConfig::pulse PkgConfig::udev ${CMAKE_DL_LIBS})
endif()

add_library(boo2 ${boo2_SRCS})
target_hsh(boo2)
target_include_directories(boo2 PUBLIC ${boo2_INCS} PRIVATE lib/audiodev/soxr/src)
target_link_libraries(boo2 PUBLIC hsh soxr logvisor ${boo2_LIBS})
target_compile_definitions(boo2 PUBLIC ${boo2_DEFS})

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  add_subdirectory(testapp)
endif()
