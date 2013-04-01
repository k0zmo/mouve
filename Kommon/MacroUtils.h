#pragma once

#include "konfig.h"

// MSVC requires another level or redirection
#if K_COMPILER == K_COMPILER_MSVC

#define K_VA_NUM_ARGS_IMPL( _1, _2, _3, _4, _5, N, ...) \
	N 
#define K_VA_NUM_ARGS_IMPL_(tuple) \
	K_VA_NUM_ARGS_IMPL tuple
#define K_VA_NUM_ARGS(...) \
	K_VA_NUM_ARGS_IMPL_((__VA_ARGS__, 5, 4, 3, 2, 1))

#define K_MACRO_DISPATCHER___(func, nargs) \
	func##nargs
#define K_MACRO_DISPATCHER__(func, nargs) \
	K_MACRO_DISPATCHER___(func, nargs)
#define K_MACRO_DISPATCHER_(func, nargs) \
	K_MACRO_DISPATCHER__(func, nargs)
#define K_MACRO_DISPATCHER(func, ...) \
	K_MACRO_DISPATCHER_(func, K_VA_NUM_ARGS(__VA_ARGS__))

#elif K_COMPILER == K_COMPILER_GCC

#define K_VA_NUM_ARGS_IMPL( _1, _2, _3, _4, _5, N, ...) \
	N 
#define K_VA_NUM_ARGS(...) \
	K_VA_NUM_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1)

#define K_MACRO_DISPATCHER__(func, nargs) \
	func##nargs
#define K_MACRO_DISPATCHER_(func, nargs) \
	K_MACRO_DISPATCHER__(func, nargs)
#define K_MACRO_DISPATCHER(func, ...) \
	K_MACRO_DISPATCHER_(func, K_VA_NUM_ARGS(__VA_ARGS__))

#endif