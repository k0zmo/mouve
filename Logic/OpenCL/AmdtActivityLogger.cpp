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

#if defined(HAVE_OPENCL)

#include "AmdtActivityLogger.h"
#include "GpuException.h"

#include <cassert>

#if defined(HAVE_AMDT_ACTIVITY_LOGGER)

#include <boost/dll/shared_library.hpp>
#include <boost/predef/os/windows.h>
#include <boost/predef/architecture/x86.h>

#include <fmt/core.h>
#include <CXLActivityLogger.h>

namespace amdt
{
    using InitializeActivityLoggerFunc = int (AL_API_CALL)();
    using BeginMarkerFunc = int (AL_API_CALL)(const char* szMarkerName,
                                                  const char* szGroupName,
                                                  const char* szUserString);
    using EndMarkerFunc = int (AL_API_CALL)();
    using FinalizeActivityLoggerFunc = int (AL_API_CALL)();

    InitializeActivityLoggerFunc* InitializeActivityLogger;
    BeginMarkerFunc* BeginMarker;
    EndMarkerFunc* EndMarker;
    FinalizeActivityLoggerFunc* FinalizeActivityLogger;

    const char* ActivityLoggerErrorString(int error)
    {
        switch(error)
        {
        case AL_UNINITIALIZED_ACTIVITY_LOGGER:
            return "Unintialized performance marker";
        case AL_FINALIZED_ACTIVITY_LOGGER:
            return "Finalized performance marker";
        case AL_UNBALANCED_MARKER:
            return "Unbalanced marker";
        case AL_APP_PROFILER_NOT_DETECTED:
            return "APP profiler not detected";
        case AL_NULL_MARKER_NAME:
            return "Null marker name";
        case AL_INTERNAL_ERROR:
            return "Internal error";
        case AL_OUT_OF_MEMORY:
            return "Out of memory";
        case AL_FAILED_TO_OPEN_OUTPUT_FILE:
            return "Failed to open output file";
        case AL_FAILED_TO_ATTACH_TO_PROFILER:
            return "Failed to attact to profiler";
        default:
            return "Unknown error";
        }
    }

    constexpr const char* libraryName =
#if BOOST_OS_WINDOWS
#  if BOOST_ARCH_X86_64
        "CXLActivityLogger-x64.dll";
#  else
        "CXLActivityLogger.dll";
#  endif
#else
        "libCXLActivityLogger.so";
#endif

} // namespace amdt

class AmdtActivityLogger : public GpuActivityLogger
{
public:
    AmdtActivityLogger()
    {
        _library.load(amdt::libraryName,
                      boost::dll::load_mode::search_system_folders);

        amdt::InitializeActivityLogger =
            _library.get<amdt::InitializeActivityLoggerFunc>(
                "amdtInitializeActivityLogger");
        amdt::BeginMarker =
            _library.get<amdt::BeginMarkerFunc>("amdtBeginMarker");
        amdt::EndMarker = _library.get<amdt::EndMarkerFunc>("amdtEndMarker");
        amdt::FinalizeActivityLogger =
            _library.get<amdt::FinalizeActivityLoggerFunc>(
                "amdtFinalizeActivityLogger");

        int error;
        if ((error = amdt::InitializeActivityLogger()) != AL_SUCCESS)
        {
            throw GpuNodeException(
                error,
                fmt::format(
                    "Error on OpenCL performance marker initialization: {}",
                    amdt::ActivityLoggerErrorString(error)));
        }
    }

    virtual ~AmdtActivityLogger()
    {
        amdt::FinalizeActivityLogger();

        amdt::InitializeActivityLogger = nullptr;
        amdt::BeginMarker = nullptr;
        amdt::EndMarker = nullptr;
        amdt::FinalizeActivityLogger = nullptr;

        _library.unload();
    }

    void beginPerfMarker(const char* markerName,
                         const char* groupName = nullptr,
                         const char* userString = nullptr) const override
    {
        assert(markerName);
        int error;
        if ((error = amdt::BeginMarker(markerName, groupName, userString)) !=
            AL_SUCCESS)
        {
            throw GpuNodeException(
                error,
                fmt::format(
                    "Error on OpenCL performance marker beginning: {}, {}",
                    markerName, amdt::ActivityLoggerErrorString(error)));
        }
    }
    void endPerfMarker() const override
    {
        int error;
        if ((error = amdt::EndMarker()) != AL_SUCCESS)
        {
            throw GpuNodeException(
                error, fmt::format("Error on OpenCL performance marker end: {}",
                                   amdt::ActivityLoggerErrorString(error)));
        }
    }

private:
    boost::dll::shared_library _library;
};

#else

using AmdtActivityLogger = NullActivityLogger;

#endif

std::unique_ptr<GpuActivityLogger> makeActivityLogger()
{
    try
    {
        return std::make_unique<AmdtActivityLogger>();
    }
    catch (std::exception&)
    {
        return std::make_unique<NullActivityLogger>();
    }
}

#endif
