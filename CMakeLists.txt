cmake_minimum_required(VERSION 3.5)

project(cosched LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libasync)

add_library(cosched STATIC scheduler.cpp include/scheduler.hpp include/scheduled_thread.hpp)
target_link_libraries(cosched PUBLIC async)
target_include_directories(cosched PUBLIC include/)

#add_executable(test test.cpp)
#target_link_libraries(test PRIVATE cosched)

install(TARGETS cosched
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
