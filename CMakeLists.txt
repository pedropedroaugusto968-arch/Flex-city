cmake_minimum_required(VERSION 3.10)
project(SpaceXitPC)

set(CMAKE_CXX_STANDARD 17)

add_executable(SpaceXit_PC main.cpp)

target_link_libraries(SpaceXit_PC d3d9 user32 gdi32 dwmapi)
