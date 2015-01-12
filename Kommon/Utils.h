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

#include <cmath>
#include <cstddef>

template<typename T>
bool fcmp(const T& a, const T& b)
{
    return std::fabs(a - b) < T(1e-8);
}

inline bool is64Bit()
{
#if K_ARCH == K_ARCH_64
    return true;
#else
    return false;
#endif
}

template <class T> struct ArraySize;
template <class T, std::size_t N> struct ArraySize<T[N]>
{
    static const std::size_t value = N;
};
#define countof(x) (ArraySize<decltype(x)>::value)

template <class Map>
auto get_or_default(const Map& m, const typename Map::key_type& key,
                    const typename Map::mapped_type& defval) -> decltype(defval)
{
    auto it = m.find(key);
    if (it == m.end())
        return defval;
    return it->second;
}

template <class T>
class SList
{
public:
    SList()
    {
        _next = _head;
        _head = this;
    }

    static T* head() { return static_cast<T*>(_head); }
    T* next() const { return static_cast<T*>(_next); }

private:
    static SList* _head;
    SList* _next;
};
template <class T> SList<T>* SList<T>::_head = nullptr;
