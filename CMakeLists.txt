cmake_minimum_required(VERSION 3.5)

project(cosched2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cosched2 STATIC
    scheduler.cpp include/cosched2/scheduler.hpp
    scheduled_thread.cpp include/cosched2/scheduled_thread.hpp
    include/cosched2/scheduler_mutex.hpp
)
target_include_directories(cosched2 PUBLIC include/)
set_target_properties(cosched2 PROPERTIES POSITION_INDEPENDENT_CODE ON)
file(GLOB_RECURSE COSCHED2_INCLUDE_FILES "include/cosched2/*.hpp")
set_target_properties(cosched2
    PROPERTIES PUBLIC_HEADER
        "include/cosched2/scheduler.hpp;include/cosched2/scheduled_thread.hpp;include/cosched2/scheduler_mutex.hpp"
)

#add_executable(test test.cpp)
#target_link_libraries(test PRIVATE cosched2)

install(TARGETS cosched2
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/cosched2
)
