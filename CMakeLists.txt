# MIT License
#
# Copyright (c) 2023 G2Labs Grzegorz Grzęda
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
cmake_minimum_required(VERSION 3.22)
enable_testing()

project(divulge)
add_library(${PROJECT_NAME})

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(${CMAKE_CURRENT_LIST_DIR}/lib/atomic-tests/cmake/atomic-tests.cmake)
    add_subdirectory(lib/containers)
    add_subdirectory(lib/g2labs-log)
    add_subdirectory(lib/encodings)
    add_subdirectory(lib/stream-server)
endif()

add_subdirectory(source)
add_subdirectory(tests)
add_subdirectory(examples)

target_link_libraries(${PROJECT_NAME} PRIVATE containers g2labs-log encodings)