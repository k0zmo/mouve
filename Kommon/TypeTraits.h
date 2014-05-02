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

#include <type_traits>

template<typename T>
struct not_
    : public std::integral_constant<bool,
        !T::value
    >
{
};

template<bool B>
struct not_bool
    : public std::integral_constant<bool,
        !B
    >
{
};

template<typename T>
struct is_scoped_enum
    : public std::integral_constant<bool,
        std::is_enum<T>::value && !std::is_convertible<T, int>::value
    >
{
};

// Works also with enum types (not like std version)
template<typename T>
struct is_unsigned_
    : public std::integral_constant<bool,
        (typename std::remove_cv<T>::type)(0) < (typename std::remove_cv<T>::type)(-1)
    >
{
};

template<typename T>
struct is_signed_
    : public std::integral_constant<bool,
        not_<is_unsigned_<T>>::value
    >
{
};

template<typename T>
struct is_enum_unsigned
    : public std::integral_constant<bool,
        is_unsigned_<T>::value &&
        std::is_enum<T>::value
    >
{
};

template<typename T>
struct is_enum_signed
    : public std::integral_constant<bool,
        is_signed_<T>::value &&
        std::is_enum<T>::value
    >
{
};

template<typename Enum>
typename std::underlying_type<Enum>::type underlying_cast(Enum e)
{
    return static_cast<typename std::underlying_type<Enum>::type>(e);
}