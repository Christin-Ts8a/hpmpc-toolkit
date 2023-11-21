cmake_minimum_required(VERSION 3.10)

project(hpmpc-toolkit)

include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(OpenSSL REQUIRED)
find_package(jsoncpp QUIET)
if(NOT jsoncpp_FOUND)
    message("jsoncpp is not found")
    find_package(Git)
    if(NOT Git_FOUND)
        message(FATAL_ERROR "Git is not found, can not initialize!")
    endif()
    execute_process(
        COMMAND git submodule update --init --external/static/jsoncpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_subdirectory(external/static/jsoncpp)
endif()

find_package(emp-tool QUIET)
if(NOT emp-tool_FOUND)
    message("emp-tool is not found")
    find_package(Git)
    if(NOT Git_FOUND)
        message(FATAL_ERROR "Git is not found, can not initialize!")
    endif()
    execute_process(
        COMMAND git submodule update --init --external/shared/emp-tool
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_subdirectory(external/shared/emp-tool)
endif()

add_definitions(-DSOURCE_DIR="${PROJECT_SOURCE_DIR}")
