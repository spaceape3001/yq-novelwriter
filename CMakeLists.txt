################################################################################
##
##  YOUR QUILL
##
################################################################################

cmake_minimum_required(VERSION 3.26.0)
project(novel LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
    if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/yq-toolbox)
        message(FATAL_ERROR "YQ toolbox must be present! (use a symbolic link or junction)")
    endif()
    include(yq-toolbox/configure.cmake)
    #   Don't yet need the toolbox....
endif()

add_subdirectory(exe)
