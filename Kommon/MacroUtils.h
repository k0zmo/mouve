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