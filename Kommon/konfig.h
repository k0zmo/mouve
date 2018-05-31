/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#pragma once

// Compiler
#define K_COMPILER_MSVC 1
#define K_COMPILER_GCC 2
#define K_COMPILER_CLANG 4

#if defined(_MSC_VER)
#  define K_COMPILER K_COMPILER_MSVC
#elif defined(__GNUC__)
#  define K_COMPILER K_COMPILER_GCC
#elif defined(__clang__)
#  define K_COMPILER K_COMPILER_CLANG
#else
#  error "This compiler is not supported"
#endif


// OS
#define K_SYSTEM_WINDOWS 1
#define K_SYSTEM_LINUX 2

#if defined(_WIN32) || defined(__WIN32__)
#  define K_SYSTEM K_SYSTEM_WINDOWS
#elif defined(linux) || defined(__linux)
#  define K_SYSTEM K_SYSTEM_LINUX
#else
#  error "This OS is not supported"
#endif


// Architecture
#define K_ARCH_32 1
#define K_ARCH_64 2

#if defined(_M_AMD64) || defined(_M_X64) || defined(__x86_64__)
#  define K_ARCH K_ARCH_64
   static_assert(sizeof(void*) == 8, "Pointer isn't 8 bytes");
#else
#  define K_ARCH K_ARCH_32
    static_assert(sizeof(void*) == 4, "Pointer isn't 4 bytes");
#endif


// Debug flag
#if defined(_DEBUG) || defined(DEBUG) && !defined(NDEBUG)
#  define K_DEBUG
#endif


// Manual debug break
#if defined(K_DEBUG)
#  if K_SYSTEM == K_SYSTEM_WINDOWS && K_COMPILER == K_COMPILER_MSVC
#    define K_DEBUG_BREAK() __debugbreak()
#  else
#    define K_DEBUG_BREAK() *(int*)(0) = 0
#  endif
#else
#  define K_DEBUG_BREAK BREAK()
#endif


// Utility macros
#define K_FILE __FILE__
#define K_LINE __LINE__
#define K_UNREFERENCED(x) (void)x;
#define K_BIT(x) (1 << x)
#define K_STRINGIFY(A) K_STRINGIFY_(A)
#define K_STRINGIFY_(A) #A

#if K_COMPILER == K_COMPILER_GCC
#  define K_FUNCTION __PRETTY_FUNCTION__
#  define K_FORCEINLINE inline __attribute__((always_inline))
#  define K_ALIGNED16(a) a __attribute__ ((aligned (16)))
#  define K_DISABLE_COPY(classname) \
    classname(const classname&) = delete; \
    classname& operator=(const classname&) = delete;
#elif K_COMPILER == K_COMPILER_MSVC
#  define K_FUNCTION __FUNCSIG__
#  define K_FORCEINLINE __forceinline
#  define K_ALIGNED16(a) __declspec(align(16)) a
#  define K_DISABLE_COPY(classname) \
    classname(const classname&); \
    classname& operator=(const classname&);
#endif


// Import/Export macros
#if K_SYSTEM == K_SYSTEM_WINDOWS
#  define K_DECL_EXPORT __declspec(dllexport)
#  define K_DECL_IMPORT __declspec(dllimport)
#elif K_SYSTEM == K_SYSTEM_LINUX
#  define K_DECL_EXPORT __attribute__((visibility("default")))
#  define K_DECL_IMPORT __attribute__((visibility("default")))
#endif

#if defined(K_STATIC_LIB)
#  define K_EXPORT
#else
#  if defined(K_BUILD_SHARED)
#    define K_EXPORT K_DECL_EXPORT
#  else
#    define K_EXPORT K_DECL_IMPORT
#  endif
#endif


// Disable specific compiler warnings
#if K_COMPILER == K_COMPILER_MSVC
// nonstandard extension used: enum used in qualified name
#  pragma warning(disable : 4482)
// class <class1> needs to have dll-interface to be used by clients of class <class2>
#  pragma warning(disable : 4251)
#endif
