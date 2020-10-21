cmake_minimum_required(VERSION 3.10 FATAL_ERROR) # because of c++17

if(BOO2_NX)
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/nx.cmake")
elseif(BOO2_MAC_CATALYST)
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/macCatalyst.cmake")
endif()

if(APPLE)
  set(OPTIONAL_OBJCXX OBJCXX)
endif()

project(boo2 LANGUAGES C CXX ${OPTIONAL_OBJCXX} VERSION 0.1)

if(CMAKE_SYSTEM_NAME STREQUAL iOS)
  foreach(lang C CXX OBJC OBJCXX)
    set(CMAKE_${lang}_COMPILER_TARGET arm64-apple-ios14)
  endforeach()
  if(CMAKE_OSX_SYSROOT AND NOT EXISTS CMAKE_OSX_SYSROOT)
    execute_process(COMMAND xcrun --sdk iphoneos --show-sdk-path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET logvisor)
  add_subdirectory(logvisor)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
if(NOT TARGET hsh)
  find_package(hsh REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
endif()

add_subdirectory(lib/audiodev/soxr/src)

set(boo2_SRCS
  lib/Boo2Implementation.cpp
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
  list(APPEND boo2_LIBS Dwmapi)
  list(APPEND boo2_SRCS lib/audiodev/AudioMatrixSSE.cpp)
elseif(APPLE)
  if(NOT BOO2_MAC_CATALYST AND metal-bin-mac IN_LIST HSH_TARGETS)
    find_library(APPKIT_LIB AppKit)
    list(APPEND boo2_LIBS ${APPKIT_LIB})
    list(APPEND boo2_INCS ${APPKIT_LIB})
  elseif(BOO2_MAC_CATALYST OR metal-bin-ios IN_LIST HSH_TARGETS)
    find_library(FOUNDATION_LIB Foundation)
    list(APPEND boo2_LIBS ${FOUNDATION_LIB})
    list(APPEND boo2_INCS ${FOUNDATION_LIB})
    find_library(UIKIT_LIB UIKit)
    list(APPEND boo2_LIBS ${UIKIT_LIB})
    list(APPEND boo2_INCS ${UIKIT_LIB})
  endif()
  list(APPEND boo2_SRCS lib/audiodev/AudioMatrixSSE.cpp)
elseif(NX)
  list(APPEND boo2_LIBS debug deko3dd optimized deko3d debug nxd optimized nx)
  list(APPEND boo2_SRCS
    lib/audiodev/AudioMatrix.cpp
    lib/audiodev/libnx.cpp
    lib/inputdev/HIDListenerNX.cpp
    lib/inputdev/HIDDeviceNX.cpp
  )
else()
  include(FindPkgConfig)
  if(TARGET PkgConfig::vk-xcb)
    # xcb already linked via hsh
    pkg_check_modules(xcb IMPORTED_TARGET xcb-icccm xcb-xkb xkbcommon-x11 xkbcommon)
    if(xcb_FOUND)
      list(APPEND boo2_LIBS PkgConfig::xcb)
    endif()
  endif()
  if(TARGET PkgConfig::vk-wayland)
    # wayland-client already linked via hsh
    pkg_check_modules(wayland IMPORTED_TARGET wayland-egl wayland-scanner wayland-protocols xkbcommon)
    if(wayland_FOUND)
      list(APPEND boo2_LIBS PkgConfig::wayland)
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
      message(STATUS "Enabling Wayland registry headers")
    endif()
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
  list(APPEND boo2_SRCS
    lib/audiodev/AudioMatrixSSE.cpp
    lib/inputdev/HIDListenerUdev.cpp
    lib/inputdev/HIDDeviceUdev.cpp
  )
  list(APPEND boo2_LIBS asound PkgConfig::pulse PkgConfig::udev ${CMAKE_DL_LIBS})
endif()

add_library(boo2 ${boo2_SRCS})
target_hsh(boo2)
target_include_directories(boo2 PUBLIC ${boo2_INCS} PRIVATE lib/audiodev/soxr/src)
target_link_libraries(boo2 PUBLIC hsh soxr logvisor ${boo2_LIBS})
target_compile_definitions(boo2 PUBLIC ${boo2_DEFS})

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  add_subdirectory(testapp)
endif()
