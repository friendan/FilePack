/* config_windows.h - Pre-built configuration for libarchive on Windows/MSVC */
#ifndef __LIBARCHIVE_CONFIG_H_INCLUDED
#define __LIBARCHIVE_CONFIG_H_INCLUDED

/* Ensure we have C99-style int64_t, etc. */
#include <stdint.h>
#include <sys/types.h>
#include <stddef.h>

/* Windows platform */
#define _WIN32 1

/* Define POSIX types missing on Windows */
#ifndef _SSIZE_T_DEFINED
typedef long long ssize_t;
#define _SSIZE_T_DEFINED
#endif
#ifndef _PID_T_DEFINED
typedef int pid_t;
#define _PID_T_DEFINED
#endif
#ifndef _UID_T_DEFINED
typedef unsigned short uid_t;
#define _UID_T_DEFINED
#endif
#ifndef _GID_T_DEFINED
typedef unsigned short gid_t;
#define _GID_T_DEFINED
#endif
#ifndef _INO_T_DEFINED
typedef unsigned short ino_t;
#define _INO_T_DEFINED
#endif
#ifndef _DEV_T_DEFINED
typedef unsigned int dev_t;
#define _DEV_T_DEFINED
#endif
#ifndef _MODE_T_DEFINED
typedef unsigned short mode_t;
#define _MODE_T_DEFINED
#endif
#ifndef _NLINK_T_DEFINED
typedef unsigned short nlink_t;
#define _NLINK_T_DEFINED
#endif

/* Headers */
#define HAVE_CTYPE_H 1
#define HAVE_DIRECT_H 1
#define HAVE_ERRNO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_IO_H 1
#define HAVE_LIMITS_H 1
#define HAVE_LOCALE_H 1
#define HAVE_MALLOC_H 1
#define HAVE_MEMORY_H 1
#define HAVE_PROCESS_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDARG_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_UTIME_H 1
#define HAVE_TIME_H 1
#define HAVE_WCHAR_H 1
#define HAVE_WCTYPE_H 1
#define HAVE_WINDOWS_H 1
#define HAVE_WINCRYPT_H 1
#define HAVE_BCRYPT_H 1
#define HAVE_WINIOCTL_H 1

/* Functions */
#define HAVE_ABORT 1
#define HAVE_ATOI 1
#define HAVE_ATOL 1
#define HAVE_CHDIR 1
#define HAVE_CLOSE 1
#define HAVE_FCHDIR 1
#define HAVE_FFLUSH 1
#define HAVE_FSEEK 1
#define HAVE_FSTAT 1
#define HAVE_FTELL 1
#define HAVE_GETCWD 1
#define HAVE_GETPID 1
#define HAVE_GMTIME_S 1
#define HAVE_LSEEK 1
#define HAVE_MBSTOWCS 1
#define HAVE_MBRTOWC 1
#define HAVE_MEMCMP 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMSET 1
#define HAVE_MKDIR 1
#define HAVE_OPEN 1
#define HAVE_READ 1
#define HAVE_RMDIR 1
#define HAVE_SETLOCALE 1
#define HAVE_SNPRINTF 1
#define HAVE_STAT 1
#define HAVE_STRCHR 1
#define HAVE_STRCPY 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRLEN 1
#define HAVE_STRNCMP 1
#define HAVE_STRRCHR 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOUL 1
#define HAVE_STRTOULL 1
#define HAVE_UNLINK 1
#define HAVE_UTIME 1
#define HAVE_WCSCMP 1
#define HAVE_WCSCPY 1
#define HAVE_WCSLEN 1
#define HAVE_WCSSTR 1
#define HAVE_WCSTOMBS 1
#define HAVE_WCRTOMB 1
#define HAVE_WRITE 1
#define HAVE_LOCALTIME_S 1
#define HAVE__FSEEKI64 1
#define HAVE__GET_TIMEZONE 1

/* Library support */
#define HAVE_LIBZ 1
#define HAVE_ZLIB_H 1

/* Integer type sizes (MSVC x64) */
#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_UNSIGNED_SHORT 2
#define SIZEOF_UNSIGNED 4
#define SIZEOF_UNSIGNED_LONG 4
#define SIZEOF_UNSIGNED_LONG_LONG 8

/* Type availability */
#define HAVE_INT16_T 1
#define HAVE_INT32_T 1
#define HAVE_INT64_T 1
#define HAVE_INTMAX_T 1
#define HAVE_UINT8_T 1
#define HAVE_UINT16_T 1
#define HAVE_UINT32_T 1
#define HAVE_UINT64_T 1
#define HAVE_UINTMAX_T 1
#define HAVE___INT64 1

/* Type definitions from stdint.h are used directly */
#define HAVE_STDINT_H 1

/* Define SSIZE_MAX for Windows */
#ifndef SSIZE_MAX
#define SSIZE_MAX ((ssize_t)(SIZE_MAX >> 1))
#endif

/* Define id_t for Windows */
#ifndef _ID_T_DEFINED
typedef unsigned short id_t;
#define _ID_T_DEFINED
#endif

/* Disable crypto/digest headers - only use Windows crypto */
#define ARCHIVE_CRYPTO_MD5_WIN 1
#undef ARCHIVE_CRYPTO_MD5_LIBC
#undef ARCHIVE_CRYPTO_RMD160_LIBC
#undef ARCHIVE_CRYPTO_SHA1_LIBC
#undef ARCHIVE_CRYPTO_SHA256_LIBC
#undef ARCHIVE_CRYPTO_SHA256_LIBC2
#undef ARCHIVE_CRYPTO_SHA256_LIBC3
#undef ARCHIVE_CRYPTO_SHA384_LIBC
#undef ARCHIVE_CRYPTO_SHA384_LIBC2
#undef ARCHIVE_CRYPTO_SHA384_LIBC3
#undef ARCHIVE_CRYPTO_SHA512_LIBC
#undef ARCHIVE_CRYPTO_SHA512_LIBC2
#undef ARCHIVE_CRYPTO_SHA512_LIBC3
#undef ARCHIVE_CRYPTO_MD5_LIBMD
#undef ARCHIVE_CRYPTO_RMD160_LIBMD
#undef ARCHIVE_CRYPTO_SHA1_LIBMD
#undef ARCHIVE_CRYPTO_SHA256_LIBMD
#undef ARCHIVE_CRYPTO_SHA512_LIBMD
#undef ARCHIVE_CRYPTO_MD5_LIBSYSTEM
#undef ARCHIVE_CRYPTO_SHA1_LIBSYSTEM
#undef ARCHIVE_CRYPTO_SHA256_LIBSYSTEM
#undef ARCHIVE_CRYPTO_SHA384_LIBSYSTEM
#undef ARCHIVE_CRYPTO_SHA512_LIBSYSTEM
#undef ARCHIVE_CRYPTO_MD5_MBEDTLS
#undef ARCHIVE_CRYPTO_SHA1_MBEDTLS
#undef ARCHIVE_CRYPTO_SHA256_MBEDTLS
#undef ARCHIVE_CRYPTO_SHA384_MBEDTLS
#undef ARCHIVE_CRYPTO_SHA512_MBEDTLS
#undef ARCHIVE_CRYPTO_MD5_NETTLE
#undef ARCHIVE_CRYPTO_RMD160_NETTLE
#undef ARCHIVE_CRYPTO_SHA1_NETTLE
#undef ARCHIVE_CRYPTO_SHA256_NETTLE
#undef ARCHIVE_CRYPTO_SHA384_NETTLE
#undef ARCHIVE_CRYPTO_SHA512_NETTLE
#undef ARCHIVE_CRYPTO_MD5_OPENSSL
#undef ARCHIVE_CRYPTO_RMD160_OPENSSL
#undef ARCHIVE_CRYPTO_SHA1_OPENSSL
#undef ARCHIVE_CRYPTO_SHA256_OPENSSL
#undef ARCHIVE_CRYPTO_SHA384_OPENSSL
#undef ARCHIVE_CRYPTO_SHA512_OPENSSL

/* Prevent archive_platform.h from redefining MSVC-defined macros */
#define HAVE_DECL_SIZE_MAX 1
#define HAVE_DECL_SSIZE_MAX 1
#define HAVE_DECL_UINT32_MAX 1
#define HAVE_DECL_INT32_MAX 1
#define HAVE_DECL_INT32_MIN 1
#define HAVE_DECL_UINT64_MAX 1
#define HAVE_DECL_INT64_MAX 1
#define HAVE_DECL_INT64_MIN 1
#define HAVE_DECL_UINTMAX_MAX 1
#define HAVE_DECL_INTMAX_MAX 1
#define HAVE_DECL_INTMAX_MIN 1

/* Use Windows CRT stat */
#define HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC 0

/* Disable all optional features */
#define ARCHIVE_ACL_AIX 0
#define ARCHIVE_ACL_DARWIN 0
#define ARCHIVE_ACL_FREEBSD 0
#define ARCHIVE_ACL_FREEBSD_NFS4 0
#define ARCHIVE_ACL_LIBACL 0
#define ARCHIVE_ACL_LIBRICHACL 0
#define ARCHIVE_ACL_SUNOS 0
#define ARCHIVE_ACL_SUNOS_NFS4 0
#define ARCHIVE_XATTR_AIX 0
#define ARCHIVE_XATTR_DARWIN 0
#define ARCHIVE_XATTR_FREEBSD 0
#define ARCHIVE_XATTR_LINUX 0

/* Version */
#define BSDCAT_VERSION_STRING ""
#define BSDCPIO_VERSION_STRING ""
#define BSDTAR_VERSION_STRING ""
#define BSDUNZIP_VERSION_STRING ""
#define LIBARCHIVE_VERSION_NUMBER 3008007
#define LIBARCHIVE_VERSION_STRING "3.8.7"

#endif /* __LIBARCHIVE_CONFIG_H_INCLUDED */
