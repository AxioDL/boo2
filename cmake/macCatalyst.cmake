set(CMAKE_SYSTEM_NAME macCatalyst)

include(Platform/Darwin-Initialize)
include(Platform/Darwin-Determine-CXX)
foreach(lang C CXX OBJC OBJCXX)
  include(CMakeDetermine${lang}Compiler)
endforeach()

set(APPLE 1)

if(NOT _CMAKE_OSX_SYSROOT_PATH)
  message(FATAL_ERROR "CMAKE_OSX_SYSROOT not set!")
endif()

if(NOT DEFINED CMAKE_MACOSX_BUNDLE)
  set(CMAKE_MACOSX_BUNDLE ON)
endif()

list(APPEND CMAKE_FIND_ROOT_PATH "${_CMAKE_OSX_SYSROOT_PATH}")
if(NOT DEFINED CMAKE_FIND_ROOT_PATH_MODE_LIBRARY)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
endif()
if(NOT DEFINED CMAKE_FIND_ROOT_PATH_MODE_INCLUDE)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()
if(NOT DEFINED CMAKE_FIND_ROOT_PATH_MODE_PACKAGE)
  set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()

macro(read_sdk_setting variable path)
  execute_process(COMMAND /usr/libexec/PlistBuddy -c
      "print ${path}"
      "${_CMAKE_OSX_SYSROOT_PATH}/SDKSettings.plist"
      OUTPUT_VARIABLE ${variable}
      RESULT_VARIABLE HAD_ERROR
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE)
endmacro()

if(CMAKE_OSX_DEPLOYMENT_TARGET)
  set(OSX_VERSION ${CMAKE_OSX_DEPLOYMENT_TARGET})
else()
  read_sdk_setting(OSX_VERSION :Version)
  if(HAD_ERROR)
    message(FATAL_ERROR "Unable to determine SDK version; please set CMAKE_OSX_DEPLOYMENT_TARGET")
  endif()
endif()

read_sdk_setting(IOS_TWIN_PREFIX_PATH :DefaultProperties:IOS_UNZIPPERED_TWIN_PREFIX_PATH)
if(HAD_ERROR)
  message(FATAL_ERROR "Unable to access IOS_UNZIPPERED_TWIN_PREFIX_PATH from SDKSettings.plist")
endif()

read_sdk_setting(IOSMAC_VERSION :Variants:1:BuildSettings:IPHONEOS_DEPLOYMENT_TARGET)
if(HAD_ERROR)
  message(FATAL_ERROR "Unable to access :Variants:1:BuildSettings:IPHONEOS_DEPLOYMENT_TARGET from SDKSettings.plist")
endif()

read_sdk_setting(TARGET_TRIPLE_SUFFIX :Variants:1:BuildSettings:LLVM_TARGET_TRIPLE_SUFFIX)
if(HAD_ERROR)
  message(FATAL_ERROR "Unable to access :Variants:1:BuildSettings:LLVM_TARGET_TRIPLE_SUFFIX from SDKSettings.plist")
endif()

set(IMPLICIT_IFRAMEWORK "${_CMAKE_OSX_SYSROOT_PATH}${IOS_TWIN_PREFIX_PATH}/System/Library/Frameworks")

foreach(lang C CXX OBJC OBJCXX)
  set(CMAKE_${lang}_COMPILER_TARGET ${CMAKE_HOST_SYSTEM_PROCESSOR}-apple-ios${IOSMAC_VERSION}${TARGET_TRIPLE_SUFFIX})

  # Set default framework search path flag for languages known to use a
  # preprocessor that may find headers in frameworks.
  set(CMAKE_${lang}_SYSROOT_FLAG -isysroot)
  set(CMAKE_${lang}_FRAMEWORK_SEARCH_FLAG -F)
  set(CMAKE_${lang}_SYSTEM_FRAMEWORK_SEARCH_FLAG "-iframework ")
  set(CMAKE_${lang}_LINK_EXECUTABLE
      "<CMAKE_${lang}_COMPILER> -iframework ${IMPLICIT_IFRAMEWORK} <FLAGS> <CMAKE_${lang}_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
endforeach()

set(CMAKE_PLATFORM_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES
    ${_CMAKE_OSX_SYSROOT_PATH}/System/Library/Frameworks
    ${_CMAKE_OSX_SYSROOT_PATH}${IOS_TWIN_PREFIX_PATH}/System/Library/Frameworks
    )

# set up the default search directories for frameworks
set(CMAKE_SYSTEM_FRAMEWORK_PATH
    ${_CMAKE_OSX_SYSROOT_PATH}/System/Library/Frameworks
    ${_CMAKE_OSX_SYSROOT_PATH}${IOS_TWIN_PREFIX_PATH}/System/Library/Frameworks
    )

if(EXISTS ${_CMAKE_OSX_SYSROOT_PATH}${IOS_TWIN_PREFIX_PATH}/usr/lib)
  list(INSERT CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES 0 ${_CMAKE_OSX_SYSROOT_PATH}${IOS_TWIN_PREFIX_PATH}/usr/lib)
endif()
list(APPEND CMAKE_SYSTEM_FRAMEWORK_PATH
    ${IOS_TWIN_PREFIX_PATH}/System/Library/Frameworks)

if(EXISTS ${_CMAKE_OSX_SYSROOT_PATH}${IOS_TWIN_PREFIX_PATH}/usr/include)
  list(INSERT CMAKE_SYSTEM_PREFIX_PATH 0 ${_CMAKE_OSX_SYSROOT_PATH}${IOS_TWIN_PREFIX_PATH}/usr)
  foreach(lang C CXX OBJC OBJCXX Swift)
    list(APPEND CMAKE_${lang}_IMPLICIT_INCLUDE_DIRECTORIES ${_CMAKE_OSX_SYSROOT_PATH}${IOS_TWIN_PREFIX_PATH}/usr/include)
  endforeach()
endif()
