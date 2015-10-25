#ifndef _UTIL_DEBUG_H
#define _UTIL_DEBUG_H

/* This module defines several debugging macros, notably LogF and Assert. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define WindowsOnly(c) c
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define WindowsOnly(c)
#endif

// LogF is a printf equivalent that also works on Windows.
#ifdef WIN32
#define LogF(...) { \
    char MessageBuffer[512]; \
    sprintf_s(MessageBuffer, 512, __VA_ARGS__); \
    OutputDebugStringA(MessageBuffer); \
}
#else
#define LogF(...) { \
    fprintf(stderr, __VA_ARGS__); \
}
#endif

// LogS is similar, but only prints a single string literal.
// This is probably more efficient on Windows.
#ifdef WIN32
#define LogS(Str) { \
    OutputDebugStringA(Str); \
}
#else
#define LogS(Str) { \
    fprintf(stderr, Str); \
}
#endif

// Log is a printf equivalent that also prints debugging information.
#define Log(...) { \
    LogF("%s(%d) %s: ", __FILENAME__, __LINE__, __FUNCTION__); \
    LogF(__VA_ARGS__); \
    LogS("\n"); \
}

// DEBUG_WindowsPrintError prints the last Windows error, if on Windows.
#if defined(WIN32) && !defined(DEBUG_NO_WINDOWS_ERRORS)
#define DEBUG_WindowsPrintError() { \
    DWORD ErrorCode = GetLastError(); \
    char ErrorMessage[256]; \
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), \
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ErrorMessage, 256, NULL); \
    LogF("Last Windows error: %s (code %d)\n", ErrorMessage, ErrorCode); \
}
#else
#define DEBUG_WindowsPrintError()
#endif

// Assert checks to see if a condition is true; if not, it stops the program and
// prints out debugging information.
// TODO: check if BUILD_NOASSERT actually does something
#ifndef DEBUG_NO_ASSERT
#define Assert(cond) { \
    if (!(cond)) { \
        LogF("Assertion failed @ %s:%d in %s\n  %s\n", __FILENAME__, __LINE__, \
            __FUNCTION__, #cond); \
        DEBUG_WindowsPrintError(); \
        exit(1); \
    } \
}
#else
#define Assert(cond)
#endif

// The following macros cause the function to return if cond is false.
// Check simply prints the failing condition and returns with no code.
// CheckM prints a specific message.
// CheckR uses a specific return code.
// CheckMR does both.
// CheckSoft is a Check equivalent that does not return.
// CheckSoftM is a CheckM equivalent that does not return.
// CheckS silently returns without printing anything.
// CheckSR silently returns a specific value.

#define Check(cond) if (!(cond)) { \
    LogF("Check failed @ %s:%d in %s\n  %s\n", __FILENAME__, __LINE__, \
        __FUNCTION__, #cond); \
    DEBUG_WindowsPrintError(); \
    return; \
}
#define CheckM(cond, msg) if (!(cond)) { \
    LogF("Check failed @ %s:%d in %s\n  %s\n", __FILENAME__, __LINE__, \
        __FUNCTION__, msg); \
    DEBUG_WindowsPrintError(); \
    return; \
}
#define CheckR(cond, retval) if (!(cond)) { \
    LogF("Check failed @ %s:%d in %s\n  %s\n", __FILENAME__, __LINE__, \
        __FUNCTION__, #cond); \
    DEBUG_WindowsPrintError(); \
    return retval; \
}
#define CheckMR(cond, msg, retval) if (!(cond)) { \
    LogF("Check failed @ %s:%d in %s\n  %s\n", __FILENAME__, __LINE__, \
        __FUNCTION__, msg); \
    DEBUG_WindowsPrintError(); \
    return retval; \
}
#define CheckSoft(cond) if (!(cond)) { \
    LogF("Check failed @ %s:%d in %s\n  %s\n", __FILENAME__, __LINE__, \
        __FUNCTION__, #cond); \
    DEBUG_WindowsPrintError(); \
}
#define CheckSoftM(cond, msg) if (!(cond)) { \
    LogF("Check failed @ %s:%d in %s\n  %s\n", __FILENAME__, __LINE__, \
        __FUNCTION__, msg); \
    DEBUG_WindowsPrintError(); \
}
#define CheckS(cond) if (!(cond)) return;
#define CheckSR(cond, retval) if (!(cond)) return retval;

#endif