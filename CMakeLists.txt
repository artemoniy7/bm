cmake_minimum_required(VERSION 3.16)
project(ApexRush VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

add_executable(apex_rush src/main.cpp)
target_link_libraries(apex_rush PRIVATE raylib)
if(APPLE)
  target_link_libraries(apex_rush PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()
if(MSVC)
  target_compile_options(apex_rush PRIVATE /W4)
else()
  target_compile_options(apex_rush PRIVATE -Wall -Wextra -Wpedantic)
endif()
