cmake_minimum_required(VERSION 3.10)
project(horoscope_fortress)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS -O2)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(horoscope_lantern lantern/main.cpp lantern/child.h)
target_link_libraries(horoscope_lantern PRIVATE Threads::Threads -static-libgcc -static-libstdc++ libseccomp.a)


add_executable(horoscope_fair fair/main.cpp fair/call_lantern.h)
target_link_libraries(horoscope_fair PRIVATE Threads::Threads -static-libgcc -static-libstdc++)

