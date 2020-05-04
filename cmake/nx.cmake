unset(CMAKE_C_COMPILER CACHE)
unset(CMAKE_CXX_COMPILER CACHE)

set(CMAKE_SYSTEM_NAME "NX")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

macro(msys_to_cmake_path MsysPath ResultingPath)
  if(WIN32)
    string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" ${ResultingPath} "${MsysPath}")
  else()
    set(${ResultingPath} "${MsysPath}")
  endif()
endmacro()

msys_to_cmake_path("$ENV{DEVKITPRO}" DEVKITPRO)

set(NX 1)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
include(nx/SwitchTools_nx)

if(WIN32)
  set(CMAKE_C_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc.exe")
  set(CMAKE_CXX_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-g++.exe")
  set(CMAKE_AR "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc-ar.exe")
  set(CMAKE_RANLIB "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc-ranlib.exe")
else()
  set(CMAKE_C_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc")
  set(CMAKE_CXX_COMPILER "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-g++")
  set(CMAKE_AR "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc-ar")
  set(CMAKE_RANLIB "${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc-ranlib")
endif()

set(CMAKE_SYSTEM_INCLUDE_PATH "${DEVKITPRO}/libnx/include")
set(CMAKE_SYSTEM_LIBRARY_PATH "${DEVKITPRO}/libnx/lib")

add_compile_definitions(__SWITCH__=1)
include_directories("${DEVKITPRO}/libnx/include")
set(CMAKE_C_FLAGS_INIT "-ffunction-sections -fdata-sections -fno-omit-frame-pointer -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=local-exec")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -fno-exceptions -fpermissive")
set(CMAKE_C_FLAGS_DEBUG_INIT "-O0")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "${CMAKE_C_FLAGS_DEBUG_INIT}")
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

set(CMAKE_FIND_ROOT_PATH / ${DEVKITPRO}/devkitA64 ${DEVKITPRO}/libnx ${DEVKITPRO})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
#set(CMAKE_STATIC_LINKER_FLAGS_INIT "-march=armv8-a -mtune=cortex-a57 -mtp=soft -L${DEVKITPRO}/libnx/lib")
set(CMAKE_EXE_LINKER_FLAGS "\"-specs=${CMAKE_CURRENT_LIST_DIR}/nx/switch-hsh.specs\" \"-T${CMAKE_CURRENT_LIST_DIR}/nx/switch-hsh.ld\" -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE -L${DEVKITPRO}/libnx/lib")


set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Shared libs not available")
set(CMAKE_INSTALL_PREFIX ${DEVKITPRO}/portlibs/switch)