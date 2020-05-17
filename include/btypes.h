/* $Id:$ */

/** @file btypes.h */

#ifndef BTYPES_H
#define BTYPES_H


#define _WIN32_WINNT 0x0500    // Windows 2000
#define _WIN32_WINDOWS 0x400   // Windows 95
#define WINVER 0x0400          // Windows NT 4.0 / Windows 95
#define _WIN32_IE_ 0x0401      // 4.01 (win98 and NT4SP5+)

#pragma warning(disable: 4244)  // 'conversion' conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable: 4761)  // integral size mismatch in argument : conversion supplied
#pragma warning(disable: 4200)  // nonstandard extension used : zero-sized array in struct/union

#pragma warning(disable: 4996)   // 'strdup' was declared deprecated
#define _CRT_SECURE_NO_DEPRECATE // all deprecated 'unsafe string functions

#define NORETURN __declspec(noreturn)
#define FORCEINLINE __forceinline
#define inline _inline

#define CPU_LITTLE_ENDIAN 1
#define CPU_BIG_ENDIAN    (!CPU_LITTLE_ENDIAN)

typedef unsigned char byte;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned int uint;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed __int64 int64;
typedef unsigned __int64 uint64;


#endif /* BTYPES_H */
