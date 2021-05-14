cmake_minimum_required(VERSION 3.19)
project(horoscope_fortress)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS -O2)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(horoscope_fortress main.cpp child.h)
target_link_libraries(horoscope_fortress PRIVATE Threads::Threads -static-libgcc -static-libstdc++ libseccomp.a)
