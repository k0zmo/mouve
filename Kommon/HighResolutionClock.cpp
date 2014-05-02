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

#include "HighResolutionClock.h"

#if K_SYSTEM == K_SYSTEM_WINDOWS
#  include <Windows.h>
#  undef max
#  undef min

HighResolutionClock::HighResolutionClock()
{
    LARGE_INTEGER freq;
    SetThreadAffinityMask(GetCurrentThread(), 1);
    QueryPerformanceFrequency(&freq);
    _periodTime = 1.0 / static_cast<double>(freq.QuadPart);
}

double HighResolutionClock::currentTimeInSeconds()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    return static_cast<double>(time.QuadPart) * _periodTime;
}

#elif K_SYSTEM == K_SYSTEM_LINUX

HighResolutionClock::HighResolutionClock()
{
    gettimeofday(&_startTime, nullptr);
}

double HighResolutionClock::currentTimeInSeconds()
{
    timeval current;
    gettimeofday(&current, nullptr);
    return current.tv_sec - _startTime.tv_sec + 
        0.000001 * (current.tv_usec - _startTime.tv_usec);
}
#endif