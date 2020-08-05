cmake_minimum_required(VERSION 3.2)

add_library(teleinfo INTERFACE)
target_include_directories(teleinfo INTERFACE ..)

add_executable(main main.cpp)
target_compile_options(main PRIVATE
    -Wall
    -Wextra)
target_link_libraries(main PRIVATE teleinfo)
