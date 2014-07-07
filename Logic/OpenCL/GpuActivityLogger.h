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

#if defined(HAVE_OPENCL)

#include "../Prerequisites.h"
#include "Kommon/SharedLibrary.h"

class LOGIC_EXPORT GpuActivityLogger
{
public:
    GpuActivityLogger();
    ~GpuActivityLogger();

    void beginPerfMarker(const char* markerName,
                         const char* groupName = nullptr,
                         const char* userString = nullptr) const;
    void endPerfMarker() const;

    bool amdtActivityLoggerInitialized() const
    {
        return _amdtActivityLoggerInitialized;
    }

private:
    LibraryHandle _libraryHandle;
    bool _amdtActivityLoggerInitialized;
};

// RAII styled performance marker for AMDT activity logger
class GpuPerformanceMarker
{
public:
    GpuPerformanceMarker(const GpuActivityLogger& logger,
                         const char* markerName,
                         const char* groupName = nullptr,
                         const char* userString = nullptr);
    ~GpuPerformanceMarker();

private:
    const GpuActivityLogger& _logger;
};

#endif