# Install script for directory: /Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/opt/homebrew/bin/arm-none-eabi-objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/sys" TYPE FILE FILES
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/auxv.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/cdefs.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/config.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/custom_file.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_default_fcntl.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/dirent.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/dir.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/errno.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/fcntl.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/features.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/file.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/iconvnls.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_initfini.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_intsup.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_locale.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/lock.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/param.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/queue.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/resource.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/sched.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/select.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_select.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_sigset.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/stat.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_stdint.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/string.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/syslimits.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/timeb.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/time.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/times.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_timespec.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/timespec.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_timeval.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/tree.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_types.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/types.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_tz_structs.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/unistd.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/utime.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/wait.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sys/_wait.h"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/yiwenxu/Projects/coursework/PESP/base-station/build/modules/picolibc/newlib/libc/include/sys/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
