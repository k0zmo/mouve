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

#include "TypeTraits.h"
#include <cstdint>

template<typename Enum>
class EnumFlags
{
    static_assert(std::is_enum<Enum>::value, "Enum is not a enum-type");
    static_assert(sizeof(Enum) <= 8, "Enum is too big");

public:
    typedef typename std::conditional<(sizeof(Enum) > 4), 
        typename std::conditional<is_enum_signed<Enum>::value, int64_t, uint64_t>::type,
        typename std::conditional<is_enum_signed<Enum>::value, int32_t, uint64_t>::type
    >::type underlying_type;
    //typedef typename std::underlying_type<Enum>::type underlying_type;
    typedef Enum enum_type;

    EnumFlags() : _v(0) {}
    EnumFlags(Enum v) : _v(static_cast<underlying_type>(v)) {}

    // Compiler-generated copy/move ctor/assignment operators are fine

    underlying_type raw() const { return _v; }
    Enum cast() const { return static_cast<Enum>(_v); }

    EnumFlags operator~() const { return EnumFlags(~_v); }
    EnumFlags operator&(EnumFlags f) const { return EnumFlags(_v & f._v); }
    EnumFlags operator|(EnumFlags f) const { return EnumFlags(_v | f._v); }
    EnumFlags operator^(EnumFlags f) const { return EnumFlags(_v ^ f._v); }

    EnumFlags& operator&=(EnumFlags f) { _v &= f._v; return *this; }
    EnumFlags& operator|=(EnumFlags f) { _v |= f._v; return *this; }
    EnumFlags& operator^=(EnumFlags f) { _v ^= f._v; return *this; }

    // explicit operator bool() const { return _v != 0; }
    bool testFlag(Enum f) const { return (_v & static_cast<underlying_type>(f)) == static_cast<underlying_type>(f); }

private:
    explicit EnumFlags(underlying_type v) : _v(v) {}

private:
    underlying_type _v;
};

#define K_DEFINE_ENUMFLAGS_OPERATORS(EnumFlagsType) \
    inline EnumFlagsType operator|(EnumFlagsType::enum_type a, EnumFlagsType b) { return b | a; } \
    inline EnumFlagsType operator&(EnumFlagsType::enum_type a, EnumFlagsType b) { return b & a; } \
    inline EnumFlagsType operator^(EnumFlagsType::enum_type a, EnumFlagsType b) { return b ^ a; } \
    inline EnumFlagsType operator|(EnumFlagsType::enum_type a, EnumFlagsType::enum_type b) { return EnumFlagsType(a) | b; } \
    inline EnumFlagsType operator&(EnumFlagsType::enum_type a, EnumFlagsType::enum_type b) { return EnumFlagsType(a) & b; } \
    inline EnumFlagsType operator^(EnumFlagsType::enum_type a, EnumFlagsType::enum_type b) { return EnumFlagsType(a) ^ b; } \
    inline EnumFlagsType operator~(EnumFlagsType::enum_type a) { return ~EnumFlagsType(a); }
