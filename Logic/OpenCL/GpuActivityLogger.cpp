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

#include "GpuActivityLogger.h"
#include "GpuException.h"

#if defined(HAVE_AMDT_ACTIVITY_LOGGER)
#  include <AMDTActivityLogger.h>

namespace amdt
{
    using InitializeActivityLoggerFuncPtr = int (AL_API_CALL*)();
    using BeginMarkerFuncPtr = int (AL_API_CALL*)(const char* szMarkerName, 
                                                  const char* szGroupName,
                                                  const char* szUserString);
    using EndMarkerFuncPtr = int (AL_API_CALL*)();
    using FinalizeActivityLoggerFuncPtr = int (AL_API_CALL*)();
    using StopTraceFuncPtr = int (AL_API_CALL*)();
    using ResumeTraceFuncPtr = int (AL_API_CALL*)();

    InitializeActivityLoggerFuncPtr InitializeActivityLogger;
    BeginMarkerFuncPtr BeginMarker;
    EndMarkerFuncPtr EndMarker;
    FinalizeActivityLoggerFuncPtr FinalizeActivityLogger;
    StopTraceFuncPtr StopTrace;
    ResumeTraceFuncPtr ResumeTrace;

    std::string ActivityLoggerErrorString(int error)
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

    LibraryHandle LoadSharedLibrary()
    {
        try
        {

            return SharedLibrary::loadLibrary(
#if K_SYSTEM == K_SYSTEM_WINDOWS
#  if K_ARCH == K_ARCH_64
                "AMDTActivityLogger-x64.dll"
#  else
                "AMDTActivityLogger.dll"
#  endif
#else
                "libAMDTActivityLogger.so"
#endif
                );
        }
        catch (std::exception&)
        {
            return static_cast<LibraryHandle>(nullptr);
        }
    }
}

#endif

GpuActivityLogger::GpuActivityLogger()
    : _libraryHandle(nullptr)
    , _amdtActivityLoggerInitialized(false)
{
#if defined(HAVE_AMDT_ACTIVITY_LOGGER)
    _libraryHandle = amdt::LoadSharedLibrary();
    if (!_libraryHandle) return;

    amdt::InitializeActivityLogger =
        SharedLibrary::getFunctionAddress<amdt::InitializeActivityLoggerFuncPtr>(
            _libraryHandle, "amdtInitializeActivityLogger");
    amdt::BeginMarker = 
        SharedLibrary::getFunctionAddress<amdt::BeginMarkerFuncPtr>(
            _libraryHandle, "amdtBeginMarker");
    amdt::EndMarker = 
        SharedLibrary::getFunctionAddress<amdt::EndMarkerFuncPtr>(
            _libraryHandle, "amdtEndMarker");
    amdt::FinalizeActivityLogger = 
        SharedLibrary::getFunctionAddress<amdt::FinalizeActivityLoggerFuncPtr>(
            _libraryHandle, "amdtFinalizeActivityLogger");
    amdt::StopTrace = 
        SharedLibrary::getFunctionAddress<amdt::StopTraceFuncPtr>(
            _libraryHandle, "amdtStopTrace");
    amdt::ResumeTrace = 
        SharedLibrary::getFunctionAddress<amdt::ResumeTraceFuncPtr>(
            _libraryHandle, "amdtResumeTrace");

    if (amdt::InitializeActivityLogger && amdt::BeginMarker && amdt::EndMarker && 
        amdt::FinalizeActivityLogger && amdt::StopTrace && amdt::ResumeTrace)
    {
        if(amdt::InitializeActivityLogger() == AL_SUCCESS)
            _amdtActivityLoggerInitialized = true;
    }
#endif
}

GpuActivityLogger::~GpuActivityLogger()
{
#if defined(HAVE_AMDT_ACTIVITY_LOGGER)
    if(_amdtActivityLoggerInitialized)
        amdt::FinalizeActivityLogger();
    if (_libraryHandle)
        SharedLibrary::unloadLibrary(_libraryHandle);
#endif
}

void GpuActivityLogger::beginPerfMarker(const char* markerName,
                                        const char* groupName,
                                        const char* userString) const
{
    assert(markerName);
#if defined(HAVE_AMDT_ACTIVITY_LOGGER)
    if(_amdtActivityLoggerInitialized)
    {
        int error;
        if((error = amdt::BeginMarker(markerName, groupName, userString)) != AL_SUCCESS)
            throw GpuNodeException(error, "Error on OpenCL performance marker beginning: " + 
                std::string(markerName) + ", " + amdt::ActivityLoggerErrorString(error));
    }
#else
    (void) groupName;
    (void) markerName;
    (void) userString;
#endif
}

void GpuActivityLogger::endPerfMarker() const
{
#if defined(HAVE_AMDT_ACTIVITY_LOGGER)
    if(_amdtActivityLoggerInitialized)
    {
        int error;
        if((error = amdt::EndMarker()) != AL_SUCCESS)
            throw GpuNodeException(error, "Error on OpenCL performance marker end: " +
                amdt::ActivityLoggerErrorString(error));
    }
#endif
}

GpuPerformanceMarker::GpuPerformanceMarker(const GpuActivityLogger& logger,
                                           const char* markerName,
                                           const char* groupName,
                                           const char* userString)
    : _logger(logger)
{
    logger.beginPerfMarker(markerName, groupName, userString);
}

GpuPerformanceMarker::~GpuPerformanceMarker()
{
    if (!std::current_exception())
    {
        // Allow to throw if its normal destruction
        _logger.endPerfMarker();
    }
    else
    {
        try
        {
            _logger.endPerfMarker();
        }
        catch (...)
        { // Don't throw an exception while already handling an exception
        }
    }
}

#endif