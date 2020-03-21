if(WIN32)
  get_filename_component(HSH_ROOT_DIR [HKEY_LOCAL_MACHINE\\Software\\hsh\\hsh] ABSOLUTE)
  if(HSH_ROOT_DIR AND NOT HSH_ROOT_DIR STREQUAL "/registry")
    list(APPEND CMAKE_PREFIX_PATH "${HSH_ROOT_DIR}")
  endif()
endif()

find_package(hsh REQUIRED CONFIG)
