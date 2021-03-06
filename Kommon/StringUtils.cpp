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

#include "konfig.h"
#include "StringUtils.h"
#include "Utils.h"

#include <cstdlib>
#include <cstdarg>
#include <memory>
#include <cstring>

std::string string_format(const char* fmt, ...)
{
    char stackBuffer[512]; // Try to first use memory from the stack
    int n = static_cast<int>(countof(stackBuffer));
    va_list ap;

#if K_COMPILER == K_COMPILER_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif
    va_start(ap, fmt);
    int fmtLength = vsnprintf(stackBuffer, n, fmt, ap);
    va_end(ap);
#if K_COMPILER == K_COMPILER_MSVC
#  pragma warning(pop)
#endif

    // Output string was truncated - fallback to heap
    if (fmtLength < 0 || fmtLength >= n)
    {
        n += std::abs(fmtLength - n + 1);
        std::unique_ptr<char[]> formatted;

        while (1)
        {
            formatted.reset(new char[n]);

#if K_COMPILER == K_COMPILER_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif
            va_start(ap, fmt);
            fmtLength = vsnprintf(formatted.get(), n, fmt, ap);
            va_end(ap);
#if K_COMPILER == K_COMPILER_MSVC
#  pragma warning(pop)
#endif

            // string truncated
            if (fmtLength < 0 || fmtLength >= n)
                n += std::abs(fmtLength - n + 1);
            else
                break;
        }

        return std::string{formatted.get()};
    }
    else
    {
        return std::string{stackBuffer};
    }
}
