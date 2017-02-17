cmake_minimum_required(VERSION 3.6)
project(DataCollector)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        src/conf.c
        src/conf.h
        src/errcode.h
        src/hash.c
        src/hash.h
        src/keyval.c
        src/keyval.h
        src/libdc.h
        src/link.c
        src/link.h
        src/list.c
        src/list.h
        src/main.c
        src/mm.h
        src/object.c
        src/object.h
        src/queue.c
        src/queue.h
        src/str.c
        src/str.h
        src/test.c
        src/thread.c
        src/thread.h)

add_executable(DataCollector ${SOURCE_FILES})