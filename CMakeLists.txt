cmake_minimum_required(VERSION 3.10 FATAL_ERROR) # because of c++17
project(boo2 LANGUAGES CXX VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET logvisor)
  add_subdirectory(logvisor)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(hsh REQUIRED)

set(boo2_SRCS lib/HshImplementation.cpp)
set(boo2_LIBS "")

if(WIN32)
elseif(APPLE)
else()
  list(APPEND boo2_LIBS xcb ${CMAKE_DL_LIBS})
endif()

add_library(boo2 ${boo2_SRCS})
target_include_directories(boo2 PUBLIC include)
target_link_libraries(boo2 PUBLIC hsh logvisor ${boo2_LIBS})

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  add_subdirectory(testapp)
endif()
