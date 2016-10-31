cmake_minimum_required(VERSION 3.5)
project(DataCollector)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        C/conf.c
        C/conf.h
        C/errcode.h
        C/hash.c
        C/hash.h
        C/keyval.c
        C/keyval.h
        C/libdc.h
        C/link.c
        C/link.h
        C/list.c
        C/list.h
        C/main.c
        C/mm.h
        C/object.c
        C/object.h
        C/queue.c
        C/queue.h
        C/str.c
        C/str.h
        C/test.c
        C/thread.c
        C/thread.h)

add_executable(DataCollector ${SOURCE_FILES})