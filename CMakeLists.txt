cmake_minimum_required(VERSION 3.1)
project(eagleview)

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /std:c++latest /permissive-")
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
endif()

include_directories(externals)

add_subdirectory(externals/tinyxml2)
add_subdirectory(src)

install(TARGETS eagleview
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
