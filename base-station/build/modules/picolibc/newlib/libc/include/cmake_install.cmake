# Install script for directory: /Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include

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

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/yiwenxu/Projects/coursework/PESP/base-station/build/modules/picolibc/newlib/libc/include/sys/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/yiwenxu/Projects/coursework/PESP/base-station/build/modules/picolibc/newlib/libc/include/machine/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/yiwenxu/Projects/coursework/PESP/base-station/build/modules/picolibc/newlib/libc/include/ssp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/yiwenxu/Projects/coursework/PESP/base-station/build/modules/picolibc/newlib/libc/include/rpc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/Users/yiwenxu/Projects/coursework/PESP/base-station/build/modules/picolibc/newlib/libc/include/arpa/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/alloca.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/argz.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/ar.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/assert.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/byteswap.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/cpio.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/ctype.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/devctl.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/dirent.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/endian.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/envlock.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/envz.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/errno.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/fastmath.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/fcntl.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/fenv.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/fnmatch.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/getopt.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/glob.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/grp.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/iconv.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/ieeefp.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/inttypes.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/langinfo.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/libgen.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/limits.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/locale.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/malloc.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/math.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/memory.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/ndbm.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/newlib.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/paths.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/picotls.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/pwd.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/regdef.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/regex.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/sched.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/search.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/setjmp.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/signal.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/spawn.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/stdint.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/stdnoreturn.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/stdlib.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/string.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/strings.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/tar.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/termios.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/time.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/uchar.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/unctrl.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/unistd.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/utime.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/utmp.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/wchar.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/wctype.h"
    "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/wordexp.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES "/Users/yiwenxu/softwares/zephyrproject/modules/lib/picolibc/newlib/libc/include/complex.h")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/yiwenxu/Projects/coursework/PESP/base-station/build/modules/picolibc/newlib/libc/include/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
