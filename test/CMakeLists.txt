#
# Little Less Protocol
# Copyright (C) 2020 Frank Mueller
#
# SPDX-License-Identifier: MIT
#

cmake_minimum_required(VERSION 2.6)

project("LittleLessProtocolTest")

# Google Test + Mock
set(GTest_ROOT /usr/src/googletest)
add_subdirectory(${GTest_ROOT} "${CMAKE_CURRENT_BINARY_DIR}/googletest")

file(GLOB libFiles ../src/*.cpp)
file(GLOB testFiles *.cpp)

# same like ArduinoIDE uses
add_compile_options(-std=gnu++11 -fpermissive -fno-exceptions -ffunction-sections -fdata-sections -fno-threadsafe-statics -Wno-error=narrowing)

add_executable(tests ${libFiles} ${testFiles})
target_include_directories(tests
    PRIVATE .
    PRIVATE ../src)
target_link_libraries(tests gtest_main gmock pthread)