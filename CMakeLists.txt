cmake_minimum_required(VERSION 3.10)

project(hpmpc-toolkit)

add_definitions(-DSOURCE_DIR="${PROJECT_SOURCE_DIR}")
include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(client fluid_cli.cpp)

find_package(OpenSSL REQUIRED)
find_package(jsoncpp QUIET)
find_package(emp-tool QUIET)

target_link_libraries(main jsoncpp emp-tool)
