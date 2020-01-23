cmake_minimum_required(VERSION 2.8)

project(Test_C)
add_executable(${PROJECT_NAME} "v4l2grab.c")
target_include_directories(${PROJECT_NAME} PRIVATE v4l2grab-master)


