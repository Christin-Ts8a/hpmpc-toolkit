cmake_minimum_required(VERSION 3.10)

project(hpmpc-toolkit)

add_definitions(-DSOURCE_DIR="${PROJECT_SOURCE_DIR}")
include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(client fluid_cli.cpp)
add_executable(sharmir_client fluid_sharmir_cli.cpp)
add_executable(server fluid_ser.cpp)
add_executable(sharmir_server fluid_sharmir_ser.cpp)

find_package(OpenSSL REQUIRED)
find_package(jsoncpp QUIET)
find_package(emp-tool QUIET)

target_link_libraries(client jsoncpp emp-tool)
target_link_libraries(sharmir_client jsoncpp emp-tool)
target_link_libraries(server jsoncpp emp-tool pthread)
target_link_libraries(sharmir_server jsoncpp emp-tool)
