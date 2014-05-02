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
#include <cassert>

template <class Type>
class Singleton
{
public:
    Singleton();
    virtual ~Singleton();

    static Type& instance();
    static Type* instancePtr();

protected:
    static Type* _singleton;

#if K_COMPILER == K_COMPILER_MSVC
private:
    Singleton(const Singleton<Type>&);
    Singleton& operator=(const Singleton<Type>&);
#elif K_COMPILER == K_COMPILER_GCC
public:
    Singleton(const Singleton<Type>&) = delete;
    Singleton& operator=(const Singleton<Type>&) = delete;
#endif
};

template <class Type> inline Singleton<Type>::Singleton()
{ assert(!_singleton); _singleton = static_cast<Type*>(this); }

template <class Type> inline Singleton<Type>::~Singleton()
{ assert(_singleton); _singleton = nullptr; }

template <class Type> inline Type& Singleton<Type>::instance()
{ assert(_singleton); return *_singleton; }

template <class Type> inline Type* Singleton<Type>::instancePtr()
{ return _singleton; }

