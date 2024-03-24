cmake_minimum_required(VERSION 3.5)

project(cosched2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cosched2 SHARED
    scheduler.cpp include/scheduler.hpp
    scheduled_thread.cpp include/scheduled_thread.hpp
    include/scheduler_mutex.hpp
)
target_include_directories(cosched2 PUBLIC include/)
set_target_properties(cosched2 PROPERTIES POSITION_INDEPENDENT_CODE ON)

#add_executable(test test.cpp)
#target_link_libraries(test PRIVATE cosched2)

install(TARGETS cosched2
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
