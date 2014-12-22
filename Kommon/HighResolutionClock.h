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

#include <chrono>

#if K_COMPILER == K_COMPILER_MSVC
long long QueryPerfFrequency();
long long QueryPerfCounter();
// Till VS2015 is released this needs to suffice
struct HighResolutionClock
{
    using rep = long long;
    using period = std::nano;
    using duration = std::chrono::nanoseconds;
    using time_point = std::chrono::time_point<HighResolutionClock>;
    static const bool is_steady = true;

    static time_point now() _NOEXCEPT
    {
        static const long long freq = QueryPerfFrequency();
        const long long cnt = QueryPerfCounter();
        const long long whole = (cnt / freq) * period::den;
        const long long part = (cnt % freq) * period::den / freq;
        return time_point{duration{whole + part}};
    }
};

#else
using HighResolutionClock = std::chrono::high_resolution_clock;
#endif

inline double convertToMilliseconds(HighResolutionClock::duration dur)
{
    using namespace std::chrono;
    long long totalMicroseconds = duration_cast<microseconds>(dur).count();
    return static_cast<double>((totalMicroseconds / 1000) +
                               (totalMicroseconds % 1000) * 1e-3);
}
