cmake_minimum_required(VERSION 3.16)
project(borderless C)

set(C_STANDARD 99)
set(C_STANDARD_REQUIRED TRUE)
set(BUILD_TYPE "Debug")

add_executable(borderless WIN32 "src/borderless.c" "borderless.rc")
