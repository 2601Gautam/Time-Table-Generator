cmake_minimum_required(VERSION 3.10)
project(TimeTableGenerator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(timetable
    src/main.cpp
    src/CourseLoader.cpp
    src/TimetableGenerator.cpp
)
target_include_directories(timetable PRIVATE include)
target_compile_options(timetable PRIVATE -Wall -Wextra)
