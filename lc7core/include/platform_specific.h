#ifndef __INC_PLATFORM_SPECIFIC_H
#define __INC_PLATFORM_SPECIFIC_H

#define PLATFORM_WIN32 0
#define PLATFORM_WIN64 1
#define PLATFORM_MACOSX 2
#define PLATFORM_LINUX 3


#if (defined(_WIN32)||defined(WIN32)) && !defined(_WIN64)
#define PLATFORM PLATFORM_WIN32
#define DLLEXPORT __declspec(dllexport)
#endif

#if (defined(_WIN32)||defined(WIN32)) && defined(_WIN64)
#define PLATFORM PLATFORM_WIN64
#define DLLEXPORT __declspec(dllexport)
#endif

#if defined(__APPLE__)&&defined(__MACH__)
#include <inttypes.h>
#define PLATFORM PLATFORM_MACOSX
#define DLLEXPORT __attribute__((visibility("default")))
#define DWORD uint32_t
#define UINT32 uint32_t
#define INT32 int32_t
#endif

#if defined(__gnu_linux__)
#include <inttypes.h>
#define PLATFORM PLATFORM_LINUX
#define DLLEXPORT __attribute__((visibility("default")))
#define DWORD uint32_t
#define UINT32 uint32_t
#define INT32 int32_t
#endif

#endif