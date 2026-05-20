################################################
# tdeclamui - ClamAV GUI for Trinity Desktop
################################################
cmake_minimum_required(VERSION 3.1)
project(tdeclamui)

### TDE setup ###############################################
include( FindPkgConfig )

# TDE macros / toolchain
if( NOT DEFINED TDE_PREFIX )
  set( TDE_PREFIX "/opt/trinity" )
endif()
list( APPEND CMAKE_MODULE_PATH "${TDE_PREFIX}/share/cmake/modules" )

include( TDESetupPaths )
tde_setup_paths( )

include( TDEMacros )

##### check for TDE ########################################
find_package( TQt )
find_package( TDE )

tde_setup_architecture_flags( )
include( TestBigEndian )
test_big_endian( WORDS_BIGENDIAN )
tde_setup_largefiles( )

##### check for required packages ##########################
find_package( PkgConfig REQUIRED )

# libnotify for desktop notifications
pkg_check_modules( LIBNOTIFY libnotify )
if( LIBNOTIFY_FOUND )
  add_definitions( -DHAVE_LIBNOTIFY )
  include_directories( ${LIBNOTIFY_INCLUDE_DIRS} )
  link_directories( ${LIBNOTIFY_LIBRARY_DIRS} )
endif()

# SQLite3 for quarantine database
pkg_check_modules( SQLITE3 REQUIRED sqlite3 )
include_directories( ${SQLITE3_INCLUDE_DIRS} )
link_directories( ${SQLITE3_LIBRARY_DIRS} )

# OpenSSL for SHA-256 hashing
pkg_check_modules( OPENSSL openssl )
if( OPENSSL_FOUND )
  add_definitions( -DHAVE_OPENSSL )
  include_directories( ${OPENSSL_INCLUDE_DIRS} )
  link_directories( ${OPENSSL_LIBRARY_DIRS} )
endif()

# LibCURL for network downloads
pkg_check_modules( LIBCURL REQUIRED libcurl )
include_directories( ${LIBCURL_INCLUDE_DIRS} )
link_directories( ${LIBCURL_LIBRARY_DIRS} )

##### compiler flags #######################################
add_definitions(
  -DHAVE_CONFIG_H
  -UTQT_NO_COMPAT
)

set(CMAKE_CXX_FLAGS "-Os -flto=auto -fstrict-aliasing -ffunction-sections -fdata-sections -fno-ident -fno-plt -fomit-frame-pointer -fno-stack-protector -fno-math-errno -fvisibility=hidden -fno-threadsafe-statics -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -Wno-deprecated-declarations -DQT_SHARED -DQT_TABLET_SUPPORT -DTQT_NO_DEBUG -DQT_NO_EXCEPTIONS -DTQT_THREAD_SUPPORT -D_REENTRANT -fpermissive -std=gnu++17")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-O1 -Wl,--gc-sections -Wl,--as-needed -s")

# Ensure no per-config C++ flags are appended by CMake.
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "")
set(CMAKE_CXX_FLAGS_MINSIZEREL "")

find_program(SSSTRIP_EXECUTABLE NAMES sstrip)

##### subdirectories #######################################
add_subdirectory( src )
# add_subdirectory( po )
# add_subdirectory( data )
