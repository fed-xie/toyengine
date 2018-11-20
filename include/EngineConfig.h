#pragma once
#ifdef _MSC_VER
#define TOY_COMPILER_VC
#endif
#if defined(__GNUC__) || defined(__MINGW32__)
#define TOY_COMPILER_GCC
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

#ifndef _DEBUG
#define NDEBUG
#endif
#include <cassert>

#if defined(_M_X64)
#define CACHELINESIZE 64 //64 bytes
#elif defined(_M_IX86)
#define CACHELINESIZE 32 //32 bytes
#else
#error("Platform not supported yet")
#endif