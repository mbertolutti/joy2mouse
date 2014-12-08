project(joy2mouse)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++1y -Wall -Wextra -Weffc++")

if("${CMAKE_BUILD_TYPE}" EQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -ggdb -DDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O4 -DNDEBUG")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_joy2mouse_SRC
    main.cpp
    xbox360_controller.cpp
)

add_executable(joy2mouse ${CMAKE_joy2mouse_SRC})
