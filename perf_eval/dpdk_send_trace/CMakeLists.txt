cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

SET(PRJ fd_chen)
PROJECT(${PRJ})

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBDPDK REQUIRED libdpdk)

SET(SOURCE_EXE 
    main.c
    port_init.c
    recv_heartbeat.c
    send_heartbeat.c
    util.c
    )

# target_compile_options(${PRJ} PRIVATE ${LIBDPDK_CFLAGS})
# target_link_libraries(${PRJ} ${LIBDPDK_LDFLAGS})

SET(CMAKE_C_FLAGS "-Wall -Wextra -g -O3 -std=gnu11 -msse4.1")

INCLUDE_DIRECTORIES($ENV{RTE_SDK}/build/include)
LINK_DIRECTORIES($ENV{RTE_SDK}/build/lib)

SET(DPDK_LIBS "-Wl,--whole-archive -ldpdk -Wl,--no-whole-archive")
# SET(DPDK_LIBS -Wl,--whole-archive 
#         libdpdk 
#         -Wl,--no-whole-archive)

ADD_EXECUTABLE(${PRJ} ${SOURCE_EXE})
TARGET_LINK_LIBRARIES(${PRJ} ${DPDK_LIBS})
TARGET_LINK_LIBRARIES(${PRJ} pthread dl rt m numa)