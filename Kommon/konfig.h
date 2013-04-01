#pragma once

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

// Compiler
#define K_COMPILER_MSVC 1
#define K_COMPILER_GCC 2
/// TODO: Support clang++

#if defined(_MSC_VER)
#  define K_COMPILER K_COMPILER_MSVC
#elif defined(__GNUC__)
#  define K_COMPILER K_COMPILER_GCC
#else
#  error "This compiler is not supported"
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

#if K_COMPILER == K_COMPILER_MSVC
// nonstandard extension used: enum used in qualified name
#  pragma warning(disable : 4482)
// class <class1> needs to have dll-interface to be used by clients of class <class2>
#  pragma warning(disable : 4251)
#endif