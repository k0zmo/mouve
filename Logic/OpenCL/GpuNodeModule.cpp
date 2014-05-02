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

#include "GpuNodeModule.h"
#include "GpuException.h"

#include <cassert>

#if defined(HAVE_CLPERFMARKER_AMD)
#  include <CLPerfMarker.h>
#endif

namespace {
string kernelsDirectory();
}

GpuNodeModule::GpuNodeModule(bool interactiveInit)
    : _maxConstantMemory(0)
    , _maxLocalMemory(0)
    , _interactiveInit(interactiveInit)
    , _perfMarkersInitialized(0)
{
}

GpuNodeModule::~GpuNodeModule()
{
#if defined(HAVE_CLPERFMARKER_AMD)
    if(_perfMarkersInitialized)
        clFinalizePerfMarkerAMD();
#endif
}

bool GpuNodeModule::initialize()
{
    bool res;
#if !defined(K_DEBUG)
    if(_interactiveInit)
        res = createInteractive();
    else
#endif
        res = createDefault();

    if(res)
    {
        static string allKernelsDirectory = kernelsDirectory() + "/";
        _library.create(_context, allKernelsDirectory);
    }

    return res;
}

bool GpuNodeModule::isInitialized()
{
    return _context.isCreated();
}

std::string GpuNodeModule::moduleName() const
{
    return "opencl";
}

#define INTEL_DEBUGGING 0

bool GpuNodeModule::createDefault()
{
#if INTEL_DEBUGGING == 1
    auto devs = clw::deviceFiltered(
        clw::Filter::PlatformVendor(clw::Vendor_Intel) 
        && clw::Filter::DeviceType(clw::Cpu));
    if(!_context.create(devs) || _context.numDevices() == 0)
        return false;
#else
    if(!_context.create(clw::EDeviceType::Default) || _context.numDevices() == 0)
        return false;
#endif
        
    return createAfterContext();
}

bool GpuNodeModule::createInteractive()
{
    if(onCreateInteractive)
    {
        vector<GpuPlatform> gpuPlatforms;
        const auto& platforms_cl = clw::availablePlatforms();
        if(platforms_cl.empty())
            return false;
        for(const auto& platform_cl : platforms_cl)
        {
            GpuPlatform gpuPlatform;
            gpuPlatform.name = platform_cl.name();

            const auto& devices_cl = clw::devices(clw::EDeviceType::All, platform_cl);
            for(const auto& device_cl : devices_cl)
                gpuPlatform.devices.emplace_back(device_cl.name());
            gpuPlatforms.emplace_back(gpuPlatform);
        }

        const auto& result = onCreateInteractive(gpuPlatforms);
        if(result.type == EDeviceType::None)
            return false;

        if(result.type != EDeviceType::Specific)
        {
            if(!_context.create(clw::EDeviceType(result.type)) || _context.numDevices() == 0)
                return false;
            return createAfterContext();
        }
        else
        {
            if(result.platform < (int) gpuPlatforms.size()
            && result.device < (int) gpuPlatforms[result.platform].devices.size())
            {
                vector<clw::Device> devices0; devices0.emplace_back(
                    clw::devices(clw::EDeviceType::All, platforms_cl[result.platform])[result.device]);
                if(!_context.create(devices0) || _context.numDevices() == 0)
                    return false;
                return createAfterContext();
            }
        }
    }

    return false;
}

vector<GpuPlatform> GpuNodeModule::availablePlatforms() const
{
    vector<GpuPlatform> gpuPlatforms;
    const auto& platforms_cl = clw::availablePlatforms();
    if(platforms_cl.empty())
        return vector<GpuPlatform>();

    for(const auto& platform_cl : platforms_cl)
    {
        GpuPlatform gpuPlatform;
        gpuPlatform.name = platform_cl.name();

        const auto& devices_cl = clw::devices(clw::EDeviceType::All, platform_cl);
        for(const auto& device_cl : devices_cl)
            gpuPlatform.devices.emplace_back(device_cl.name());
        gpuPlatforms.emplace_back(gpuPlatform);
    }

    return gpuPlatforms;
}

KernelID GpuNodeModule::registerKernel(const string& kernelName,
                                       const string& programName,
                                       const string& buildOptions)
{
    string opts = additionalBuildOptions(programName);
    return _library.registerKernel(kernelName, programName, buildOptions + opts);
}

clw::Kernel GpuNodeModule::acquireKernel(KernelID kernelId)
{
    return _library.acquireKernel(kernelId);
}

KernelID GpuNodeModule::updateKernel(KernelID kernelId,
                                     const string& buildOptions)
{
    //string opts = additionalBuildOptions(programName);
    return _library.updateKernel(kernelId, buildOptions);
}

vector<GpuRegisteredProgram> GpuNodeModule::populateListOfRegisteredPrograms() const
{
    return _library.populateListOfRegisteredPrograms();
}

void GpuNodeModule::rebuildProgram(const string& programName)
{
    _library.rebuildProgram(programName);
}

bool GpuNodeModule::createAfterContext()
{
    _device = _context.devices()[0];
    _queue = _context.createCommandQueue(_device);//clw::ECommandQueueProperty::ProfilingEnabled);
    _dataQueue = _context.createCommandQueue(_device);//clw::ECommandQueueProperty::ProfilingEnabled);

    clw::installErrorHandler([=](cl_int error_id, const std::string& message)
    {
        if(error_id != CL_BUILD_PROGRAM_FAILURE)
            throw GpuNodeException(error_id, message);
    });

    if(!_device.isNull())
    {
        _maxConstantMemory = _device.maximumConstantBufferSize();
        _maxLocalMemory = _device.localMemorySize();
    }

#if defined(HAVE_CLPERFMARKER_AMD)
    if(clInitializePerfMarkerAMD() == AP_SUCCESS)
        _perfMarkersInitialized = true;
#endif

    return !_device.isNull() && !_queue.isNull();
}

#pragma region Kernels Directory

#if K_SYSTEM == K_SYSTEM_WINDOWS

#include <Windows.h>
#include <QFileInfo>
#include <QDir>

namespace {

HMODULE moduleHandle()
{
    static int s_somevar = 0;
    MEMORY_BASIC_INFORMATION mbi;
    if(!::VirtualQuery(&s_somevar, &mbi, sizeof(mbi)))
    {
        return NULL;
    }
    return static_cast<HMODULE>(mbi.AllocationBase);
}

string kernelsDirectory()
{
    HMODULE hModule = moduleHandle();
    char buffer[MAX_PATH];
    /*DWORD dwSize = */GetModuleFileNameA(hModule, buffer, MAX_PATH);
    //dwSize = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    QFileInfo fi(QString::fromLatin1(buffer));
    QDir dllDir = fi.absoluteDir();
    if(!dllDir.cd("kernels"))
        // fallback to dll directory
        return dllDir.absolutePath().toStdString();
    return dllDir.absolutePath().toStdString();
}

}

#elif K_SYSTEM == K_SYSTEM_LINUX

namespace {
string kernelsDirectory()
{
    return "./kernels";
}
}

#endif

#pragma endregion

string GpuNodeModule::additionalBuildOptions(const std::string& programName) const
{
    string opts;
#if defined(K_DEBUG) && K_COMPILER == K_COMPILER_MSVC
    // Enable kernel debugging if device is CPU and it's Intel platform
    if(_device.platform().vendorEnum() == clw::EPlatformVendor::Intel
        && _device.deviceType() == clw::EDeviceType::Cpu)
    {
        QFileInfo thisFile(K_FILE);
        QDir thisDirectory = thisFile.dir();
        QString s = thisDirectory.dirName();
        if(thisDirectory.cd("kernels"))
        {
            QString fullKernelsPath = thisDirectory.absoluteFilePath(QString::fromStdString(programName));
            opts += " -g -s " + fullKernelsPath.toStdString();
        }
    }
#else
    (void) programName;
#endif
    return opts;
}

#if defined(HAVE_CLPERFMARKER_AMD)

std::string clPerfMarkerErrorString(int error)
{
    switch(error)
    {
    case AP_UNINITIALIZED_PERF_MARKER: return "Unintialized performance marker";
    case AP_FINALIZED_PERF_MARKER: return "Finalized performance marker";
    case AP_UNBALANCED_MARKER: return "Unbalanced marker";
    case AP_APP_PROFILER_NOT_DETECTED: return "APP profiler not detected";
    case AP_NULL_MARKER_NAME: return "Null marker name";
    case AP_INTERNAL_ERROR: return "Internal error";
    case AP_OUT_OF_MEMORY: return "Out of memory";
    case AP_FAILED_TO_OPEN_OUTPUT_FILE: return "Failed to open output file";
    default: return "Unknown error";
    }
}

#endif

void GpuNodeModule::beginPerfMarker(const char* markerName,
                                    const char* groupName)
{
    assert(markerName);
#if defined(HAVE_CLPERFMARKER_AMD)
    if(_perfMarkersInitialized)
    {
        int error;
        if((error = clBeginPerfMarkerAMD(markerName, groupName)) != AP_SUCCESS)
            throw GpuNodeException(error, "Error on OpenCL performance marker beginning: " + 
                std::string(markerName) + ", " + clPerfMarkerErrorString(error));
    }
#else
    (void) groupName;
    (void) markerName;
#endif
}

void GpuNodeModule::endPerfMarker()
{
#if defined(HAVE_CLPERFMARKER_AMD)
    if(_perfMarkersInitialized)
    {
        int error;
        if((error = clEndPerfMarkerAMD()) != AP_SUCCESS)
            throw GpuNodeException(error, "Error on OpenCL performance marker end: " +
                clPerfMarkerErrorString(error));
    }
#endif
}

GpuPerformanceMarker::GpuPerformanceMarker(bool perfMarkersInitialized,
                                           const char* markerName,
                                           const char* groupName)
    : _ok(false)
{
    assert(markerName);
#if defined(HAVE_CLPERFMARKER_AMD)
    if(perfMarkersInitialized)
    {
        int error;
        if((error = clBeginPerfMarkerAMD(markerName, groupName)) != AP_SUCCESS)
            throw GpuNodeException(error, "Error on OpenCL performance marker beginning: " + 
                std::string(markerName) + ", " + clPerfMarkerErrorString(error));
        _ok = error == AP_SUCCESS;
    }
#else
    (void) perfMarkersInitialized;
    (void) markerName;
    (void) groupName;
#endif
}

GpuPerformanceMarker::~GpuPerformanceMarker()
{
#if defined(HAVE_CLPERFMARKER_AMD)
    if(_ok)
    {
        int error;
        if((error = clEndPerfMarkerAMD()) != AP_SUCCESS)
            throw GpuNodeException(error, "Error on OpenCL performance marker end: " +
                clPerfMarkerErrorString(error));
    }
#endif
}

std::unique_ptr<IGpuNodeModule> createGpuModule()
{
    return std::unique_ptr<IGpuNodeModule>(new GpuNodeModule(true));
}

#else

#include "IGpuNodeModule.h"

std::unique_ptr<IGpuNodeModule> createGpuModule()
{
    return nullptr;
}

#endif
