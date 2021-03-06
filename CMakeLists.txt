PROJECT(esesc) 
INCLUDE("${esesc_SOURCE_DIR}/CMake.common")
SET(CMAKE_MODULE_PATH "${esesc_SOURCE_DIR}/cmake-modules/conf")
cmake_minimum_required(VERSION 2.7)

#IF(ENABLE_PEQ)
#  MESSAGE("  -DENABLE_PEQ=1                Enable Equation based power model support")
#ELSE(ENABLE_PEQ)
#  MESSAGE("  -DENABLE_PEQ=0                Disable Equation based power model Fusupport (default)")
#ENDIF(ENABLE_PEQ)

IF(ENABLE_LIVE)
  MESSAGE("  -DESESC_LIVE=1                Enable esesc live sampling support")
ELSE(ENABLE_LIVE)
  MESSAGE("  -DESESC_LIVE=0                Disable esesc live compilation (default)")
ENDIF(ENABLE_LIVE)

IF(CMAKE_ESESC_QEMU_ISA MATCHES "armv7") 
  MESSAGE("  -DESESC_QEMU_ISA=armv7        qemu/armv7 ISA")
ELSE(CMAKE_ESESC_QEMU_ISA MATCHES "armv7") 
  MESSAGE("  -DESESC_QEMU_ISA=mips         qemu/mips64r6 ISA")
ENDIF(CMAKE_ESESC_QEMU_ISA MATCHES "armv7") 

#############
MESSAGE("  -DCMAKE_HOST_MARCH=${CMAKE_HOST_MARCH} compilation")

#############
# Debug vs Release
IF(CMAKE_BUILD_TYPE MATCHES "Debug")
  MESSAGE("  -DCMAKE_BUILD_TYPE=Debug      debug compilation options")
ELSE(CMAKE_BUILD_TYPE MATCHES "Debug")
  MESSAGE("  -DCMAKE_BUILD_TYPE=Release    release compilation options (default)")
ENDIF(CMAKE_BUILD_TYPE MATCHES "Debug")

################################################
# use cmake -DDEBUG=1 to activate debugging
FIND_PACKAGE(ZLIB)
FIND_PACKAGE(Threads)
FIND_PACKAGE(Curses)
FIND_PACKAGE(FLEX)
FIND_PACKAGE(BISON)
FIND_PACKAGE(X11)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# OS X Does not need SDL to run QEMU
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  FIND_LIBRARY(SDL_LIBRARIES NAMES SDL)
  FIND_LIBRARY(RT_LIBRARIES NAMES rt)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

FIND_LIBRARY(MLIB_LIBRARIES NAMES m)
FIND_LIBRARY(UTIL_LIBRARIES NAMES util)

#################################
ADD_SUBDIRECTORY(misc/libsuc)

include(ExternalProject)

ExternalProject_Add(regression
  SOURCE_DIR ${esesc_SOURCE_DIR}/gold
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/gold/configure ${esesc_SOURCE_DIR}/gold ${CMAKE_CURRENT_BINARY_DIR}/gold
  PREFIX ${esesc_SOURCE_DIR}/gold
  BUILD_COMMAND sh gen-regression.sh ${BENCHSUITS_DIR}
  BUILD_IN_SOURCE "" 
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold/stampdir
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
  )
set_target_properties(regression PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(gold
  SOURCE_DIR ${esesc_SOURCE_DIR}/gold
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/gold/configure ${esesc_SOURCE_DIR}/gold ${CMAKE_CURRENT_BINARY_DIR}/gold
  PREFIX ${esesc_SOURCE_DIR}/gold
  BUILD_COMMAND sh gen-gold.sh ${BENCHSUITS_DIR}
  BUILD_IN_SOURCE "" 
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/gold/stampdir
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
  )
set_target_properties(gold PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(floorplan
  SOURCE_DIR ${esesc_SOURCE_DIR}/pwth/floorplan
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/pwth/floorplan/configure ${esesc_SOURCE_DIR}/pwth/floorplan ${CMAKE_CURRENT_BINARY_DIR}/floorplan
  PREFIX ${esesc_SOURCE_DIR}/pwth/floorplan
  BUILD_COMMAND ${MAKE}
  BUILD_IN_SOURCE 0
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/floorplan
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/floorplan/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/floorplan/stampdir
  DOWNLOAD_COMMAND ""
  INSTALL_COMMAND ""
  )
set_target_properties(floorplan PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(bfd
  SOURCE_DIR ${esesc_SOURCE_DIR}/misc/bfd
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/misc/bfd/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/bfd CFLAGS=${CMAKE_C_FLAGS}\ -g\ -I${esesc_SOURCE_DIR}/misc/include --enable-64-bit-bfd
  PREFIX ${esesc_SOURCE_DIR}/misc/bfd
  BUILD_COMMAND ${MAKE}
  BUILD_IN_SOURCE 0
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bfd
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/bfd/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/bfd/stampdir
  DOWNLOAD_COMMAND ""
  )
set_target_properties(bfd PROPERTIES EXCLUDE_FROM_ALL TRUE)
set(bfd_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/bfd/libbfd.a)

ExternalProject_Add(opcodes
  SOURCE_DIR ${esesc_SOURCE_DIR}/misc/opcodes
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/misc/opcodes/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/opcodes CFLAGS=${CMAKE_C_FLAGS}\ -I${esesc_SOURCE_DIR}/misc/include
  PREFIX ${esesc_SOURCE_DIR}/misc/opcodes
  DEPENDS bfd
  BUILD_COMMAND ${MAKE}
  BUILD_IN_SOURCE 0
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/opcodes
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/opcodes/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/opcodes/stampdir
  DOWNLOAD_COMMAND ""
  )
set_target_properties(opcodes PROPERTIES EXCLUDE_FROM_ALL TRUE)
set(opcodes_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/opcodes/libopcodes.a)

ExternalProject_Add(libiberty
  SOURCE_DIR ${esesc_SOURCE_DIR}/misc/libiberty
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/misc/libiberty/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/libiberty CFLAGS=${CMAKE_C_FLAGS}\ -I${esesc_SOURCE_DIR}/misc/include
  PREFIX ${esesc_SOURCE_DIR}/misc/libiberty
  DEPENDS bfd
  BUILD_COMMAND ${MAKE}
  BUILD_IN_SOURCE 0
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libiberty
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/libiberty/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/libiberty/stampdir
  DOWNLOAD_COMMAND ""
  )
set_target_properties(libiberty PROPERTIES EXCLUDE_FROM_ALL TRUE)
set(libiberty_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/libiberty/libiberty.a)

IF(CMAKE_BUILD_TYPE MATCHES "Debug")
  SET(QEMU_EXTRA --enable-debug --cc=${CMAKE_C_COMPILER})
ELSE(CMAKE_BUILD_TYPE MATCHES "Debug")
  SET(QEMU_EXTRA --cc=${CMAKE_C_COMPILER})
ENDIF(CMAKE_BUILD_TYPE MATCHES "Debug")

ExternalProject_Add(qemu
  SOURCE_DIR ${esesc_SOURCE_DIR}/emul/qemu
  #CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/emul/qemu/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/qemu --target-list=arm-linux-user --enable-esesc_user --python=python2 --disable-docs --disable-smartcard --disable-guest-base 
  CONFIGURE_COMMAND ${esesc_SOURCE_DIR}/emul/qemu/configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/qemu --target-list=mips64el-linux-user --disable-docs --enable-esesc --python=python2 --disable-system --disable-tools --without-pixman --disable-werror --disable-guest-agent ${QEMU_EXTRA}
  PREFIX ${esesc_SOURCE_DIR}/emul/qemu
  BUILD_COMMAND ${MAKE}
  BUILD_IN_SOURCE 0
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/qemu
  TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/qemu/tmp
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/qemu/stampdir
  DOWNLOAD_COMMAND ""
  )
set_target_properties(qemu PROPERTIES EXCLUDE_FROM_ALL TRUE)

#################################
#ADD_SUBDIRECTORY(misc/libmigdb)
ADD_SUBDIRECTORY(emul/libemulint)
ADD_SUBDIRECTORY(emul/libqemuint)
#################################
ADD_SUBDIRECTORY(pwth/libpeq)
ADD_SUBDIRECTORY(pwth/libmcpat)
ADD_SUBDIRECTORY(pwth/libsesctherm)
ADD_SUBDIRECTORY(pwth/libpwrmodel)
#################################
ADD_SUBDIRECTORY(simu/libcore)
ADD_SUBDIRECTORY(simu/libsampler)
ADD_SUBDIRECTORY(simu/libmem)
ADD_SUBDIRECTORY(simu/libnet)
#################################
ADD_SUBDIRECTORY(main)
ADD_SUBDIRECTORY(tests/gtest-1.7.0)
ADD_SUBDIRECTORY(tests)


