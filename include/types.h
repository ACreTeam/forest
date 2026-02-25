#ifndef TYPES_H
#define TYPES_H

#include <math.h>
#include <float.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <intrin.h>
#include <dolphin/types.h>
#include "macros.h"

#ifdef TARGET_PC
#define BSWAP16(x) _byteswap_ushort(x)
#define BSWAP32(x) _byteswap_ulong(x)
#define BSWAP64(x) _byteswap_uint64(x)
#else
#define BSWAP16(x) x
#define BSWAP32(x) x
#define BSWAP64(x) x
#endif

#ifdef TARGET_PC
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)
#else
#define DLLEXPORT
#define DLLIMPORT
#endif

/* REL (DLL) imports these from the DOL (EXE); DOL defines and exports them */
#if defined(TARGET_PC)
#  if defined(IS_REL)
#    define FOREST_API DLLIMPORT
#  else
#    define FOREST_API DLLEXPORT
#  endif
#else
#  define FOREST_API
#endif

#define VER_GAFE01_00 0
#define VER_GAFU01_00 1

// TODO: add PAL version to this check
// Adjusts values based on regional frame rate (Hz)
#if VERSION == VER_GAFU01_00
#define FRAMERATE_ADJ(n) (((n) * 60) / 50) // less frames so more value
#define FRAMERATE_TIMER(n) (((n) * 50) / 60) // less frames so less timer
#define FRAMERATE_SELECT(f60, f50) (f50)
#else
#define FRAMERATE_ADJ(n) (n)
#define FRAMERATE_TIMER(n) (n)
#define FRAMERATE_SELECT(f60, f50) (f60)
#endif

typedef unsigned int uint;
typedef void unknown;

// #ifdef __MWERKS__
// #define AT_ADDRESS(x) : (x)
// #else
// #define AT_ADDRESS(x) __attribute__((section(".data." #x)))
// #endif

#define ALIGN_PREV(u, align) (u & (~(align - 1)))
#define ALIGN_NEXT(u, align) ((u + (align - 1)) & (~(align - 1)))
#define IS_ALIGNED(X, N) (((X) & ((N) - 1)) == 0)
#define IS_NOT_ALIGNED(X, N) (((X) & ((N) - 1)) != 0)

#define FLAG_ON(V, F) (((V) & (F)) == 0)
#define FLAG_OFF(V, F) (((V) & (F)) != 0)

#ifndef ATTRIBUTE_ALIGN
#if defined(__MWERKS__) || defined(__GNUC__)
#define ATTRIBUTE_ALIGN(num) __attribute__((aligned(num)))
#elif defined(_MSC_VER)
#define ATTRIBUTE_ALIGN(num)
#else
#error unknown compiler
#endif
#endif

/* Symbol export attribute for shared library (DLL) boundaries */
#ifndef DLLEXPORT
#if defined(__MWERKS__)
#define DLLEXPORT __declspec(export)
#elif defined(_MSC_VER) || (defined(__clang__) && defined(_WIN32))
#define DLLEXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define DLLEXPORT __attribute__((visibility("default")))
#else
#define DLLEXPORT
#endif
#endif

#define BUTTON_NONE 0x0000
#define BUTTON_CRIGHT 0x0001
#define BUTTON_CLEFT 0x0002
#define BUTTON_CDOWN 0x0004
#define BUTTON_CUP 0x0008
#define BUTTON_R 0x0010
#define BUTTON_L 0x0020
#define BUTTON_X 0x0040
#define BUTTON_Y 0x0080
#define BUTTON_DRIGHT 0x0100
#define BUTTON_DLEFT 0x0200
#define BUTTON_DDOWN 0x0400
#define BUTTON_DUP 0x0800
#define BUTTON_START 0x1000
#define BUTTON_Z 0x2000
#define BUTTON_B 0x4000
#define BUTTON_A 0x8000

#define FRAMES_PER_SECOND 60
#define FRAMES_PER_MINUTE (FRAMES_PER_SECOND * 60)

#define ARRAY_SIZE(arr, type) (sizeof(arr) / sizeof(type))
#define ARRAY_COUNT(arr) (int)(sizeof(arr) / sizeof(arr[0]))

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define FLOOR(n, f) (((n) / (f)) * (f))

#define F32_IS_ZERO(v) (fabsf(v) < 0.008f)

#define DECREMENT_TIMER(timer) ((timer) == 0 ? 0 : --(timer))

/* ARGB8 color format (32 bits) to RGB5A3 color format (16 bits) */
#define ARGB8_to_RGB5A3(argb8)                                                                                     \
    ((u16)(((argb8) & 0xFF000000) >= 0xE0000000                                                                    \
               ? /* Fully opaque, 5 bits per color channel */ (0x8000 | ((((argb8) >> 16) & 0xF8) << 7) |          \
                                                               ((((argb8) >> 8) & 0xF8) << 2) |                    \
                                                               (((argb8) & 0xFF) >> 3))                            \
               : /* 3 bits of transparency, 4 bits per color channel */ (                                          \
                     ((((argb8) >> 24) & 0xE0) << 7) | ((((argb8) >> 16) & 0xF0) << 4) | (((argb8) >> 8) & 0xF0) | \
                     (((argb8) & 0xF0) >> 4))))

#define GPACK_RGB5A3(r, g, b, a) \
    ARGB8_to_RGB5A3((((a) & 0xFF) << 24) | (((r) & 0xFF) << 16) | (((g) & 0xFF) << 8) | ((b) & 0xFF))

#define FORCESTRIP

#if defined(MUST_MATCH) && defined(__MWERKS__)
#define MATCH_FORCESTRIP FORCESTRIP
#else
#define MATCH_FORCESTRIP
#endif

// #if DEBUG
// #define ASSERTLINE(line, cond) \
//     ((cond) || (OSPanic(__FILE__, line, "Failed assertion " #cond), 0))

// #define ASSERTMSGLINE(line, cond, msg) \
//     ((cond) || (OSPanic(__FILE__, line, msg), 0))

// // This is dumb but we dont have a Metrowerks way to do variadic macros in the macro to make this done in a not scrubby way.
// #define ASSERTMSG1LINE(line, cond, msg, arg1) \
//     ((cond) || (OSPanic(__FILE__, line, msg, arg1), 0))
    
// #define ASSERTMSG2LINE(line, cond, msg, arg1, arg2) \
//     ((cond) || (OSPanic(__FILE__, line, msg, arg1, arg2), 0))

// #define ASSERTMSGLINEV(line, cond, ...) \
//     ((cond) || (OSPanic(__FILE__, line, __VA_ARGS__), 0))

// #else
// #define ASSERTLINE(line, cond) (void)0
// #define ASSERTMSGLINE(line, cond, msg) (void)0
// #define ASSERTMSG1LINE(line, cond, msg, arg1) (void)0
// #define ASSERTMSG2LINE(line, cond, msg, arg1, arg2) (void)0
// #define ASSERTMSGLINEV(line, cond, ...) (void)0
// #endif

#endif
