cmake_minimum_required(VERSION 3.10)

project(hpmpc-toolkit)

include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(OpenSSL REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(emp-tool REQUIRED)
