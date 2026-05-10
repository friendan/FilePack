#
cmake_minimum_required(VERSION 3.17 FATAL_ERROR)
PROJECT(libarchive C)

SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build/cmake")

# 禁用所有外部依赖（除了 zlib）
set(ENABLE_ZLIB ON)
set(ENABLE_BZip2 OFF)
set(ENABLE_LZ4 OFF)
set(ENABLE_LZO OFF)
set(ENABLE_LZMA OFF)
set(ENABLE_ZSTD OFF)
set(ENABLE_OPENSSL OFF)
set(ENABLE_LIBXML2 OFF)
set(ENABLE_EXPAT OFF)
set(ENABLE_PCREPOSIX OFF)
set(ENABLE_PCRE2POSIX OFF)
set(ENABLE_LIBGCC OFF)
set(ENABLE_LIBB2 OFF)
set(ENABLE_NETTLE OFF)
set(ENABLE_MBEDTLS OFF)
set(ENABLE_ICONV OFF)
set(ENABLE_XATTR OFF)
set(ENABLE_ACL OFF)
set(ENABLE_CNG OFF)
set(ENABLE_TAR OFF)
set(ENABLE_CPIO OFF)
set(ENABLE_CAT OFF)
set(ENABLE_UNZIP OFF)
set(ENABLE_TEST OFF)
set(ENABLE_INSTALL OFF)
set(POSIX_REGEX_LIB "NONE")
set(WINDOWS_VERSION "WIN10")

# ZLIB - 直接链接 zlibstatic 目标
set(HAVE_LIBZ 1)
set(HAVE_ZLIB_H 1)
include_directories("${CMAKE_SOURCE_DIR}/zlib/zlib-1.3.2")

# 跳过所有 POSIX 函数检测，直接设置 Windows 默认值
set(HAVE_STRCHR 1)
set(HAVE_STRLEN 1)
set(HAVE_STRNCMP 1)
set(HAVE_STRCPY 1)
set(HAVE_STRRCHR 1)
set(HAVE_GETPID 1)
set(HAVE_OPEN 1)
set(HAVE_CLOSE 1)
set(HAVE_READ 1)
set(HAVE_WRITE 1)
set(HAVE_LSEEK 1)
set(HAVE_STAT 1)
set(HAVE_FSTAT 1)
set(HAVE_UTIME 1)
set(HAVE_LOCALTIME_S 1)
set(HAVE_GMTIME_S 1)
set(HAVE_MEMSET 1)
set(HAVE_MEMCPY 1)
set(HAVE_MEMMOVE 1)
set(HAVE_MEMCMP 1)
set(HAVE_ABORT 1)
set(HAVE_SNPRINTF 1)
set(HAVE_MKDIR 1)
set(HAVE_STRDUP 1)
set(HAVE_STRERROR 1)
set(HAVE_SETLOCALE 1)
set(HAVE_WCSLEN 1)
set(HAVE_WCSCPY 1)
set(HAVE_WCSCMP 1)
set(HAVE_WCSSTR 1)
set(HAVE_WCRTOMB 1)
set(HAVE_MBRTOWC 1)

# 头文件检测
set(HAVE_SYS_TYPES_H 1)
set(HAVE_SYS_STAT_H 1)
set(HAVE_SYS_UTIME_H 1)
set(HAVE_STDINT_H 1)
set(HAVE_STDIO_H 1)
set(HAVE_STDLIB_H 1)
set(HAVE_STRING_H 1)
set(HAVE_WCHAR_H 1)
set(HAVE_CTYPE_H 1)
set(HAVE_ERRNO_H 1)
set(HAVE_FCNTL_H 1)
set(HAVE_LIMITS_H 1)
set(HAVE_LOCALE_H 1)
set(HAVE_MALLOC_H 1)
set(HAVE_MEMORY_H 1)
set(HAVE_SIGNAL_H 1)
set(HAVE_TIME_H 1)
set(HAVE_DIRECT_H 1)
set(HAVE_IO_H 1)
set(HAVE_PROCESS_H 1)
set(HAVE_INTTYPES_H 1)
set(HAVE_WINDOWS_H 1)
set(HAVE_WINCRYPT_H 1)
set(HAVE_WINIOCTL_H 1)

# 类型定义 - MSVC x64
set(SIZEOF_SHORT 2)
set(SIZEOF_INT 4)
set(SIZEOF_LONG 4)
set(SIZEOF_LONG_LONG 8)
set(SIZEOF___INT64 8)
set(HAVE_INT16_T 1)
set(HAVE_INT32_T 1)
set(HAVE_INT64_T 1)
set(HAVE_UINT8_T 1)
set(HAVE_UINT16_T 1)
set(HAVE_UINT32_T 1)
set(HAVE_UINT64_T 1)
set(INT64_T "__int64")
set(UINT64_T "unsigned __int64")
set(INT32_T "int")
set(UINT32_T "unsigned int")

set(ADDITIONAL_LIBS zlibstatic)

# 直接把预制的 config_windows.h 复制为 config.h
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/config_windows.h"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h"
    COPYONLY
)

# 收集 libarchive 核心源文件（排除 POSIX 专用文件）
file(GLOB LIBARCHIVE_SOURCES
    "libarchive/archive_*.c"
    "libarchive/filter_fork_windows.c"
    "libarchive/xxhash.c"
)
# 排除 POSIX 平台专用源文件
list(REMOVE_ITEM LIBARCHIVE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/archive_disk_acl_darwin.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/archive_disk_acl_freebsd.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/archive_disk_acl_linux.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/archive_disk_acl_sunos.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/archive_read_disk_posix.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/archive_read_disk_set_standard_lookup.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/archive_write_disk_posix.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/archive_write_disk_set_standard_lookup.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/filter_fork_posix.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/archive_blake2sp_ref.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/libarchive/archive_blake2s_ref.c"
)

# 包含路径
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(libarchive)

# 静态库
add_library(archive_static STATIC ${LIBARCHIVE_SOURCES})
target_compile_definitions(archive_static PRIVATE
    LIBARCHIVE_STATIC
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
    PLATFORM_CONFIG_H="config.h"
)
target_include_directories(archive_static PUBLIC libarchive ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(archive_static PUBLIC ${ADDITIONAL_LIBS})

if(MSVC)
    set_target_properties(archive_static PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()
