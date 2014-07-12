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

namespace {
string kernelsDirectory();
}

GpuNodeModule::GpuNodeModule(bool interactiveInit)
    : _maxConstantMemory(0)
    , _maxLocalMemory(0)
    , _logger()
    , _interactiveInit(interactiveInit)
{
}

GpuNodeModule::~GpuNodeModule()
{
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
        clw::Filter::PlatformVendor(clw::EPlatformVendor::Intel) 
        && clw::Filter::DeviceType(clw::EDeviceType::Cpu));
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

    return !_device.isNull() && !_queue.isNull();
}

size_t GpuNodeModule::warpSize() const
{
    if(device().deviceType() != clw::EDeviceType::Gpu)
    {
        return 1;
    }
    else
    {
        clw::EPlatformVendor vendor = device().platform().vendorEnum();

        if(vendor == clw::EPlatformVendor::AMD)
            return device().wavefrontWidth();
        // How to get this for Intel HD Graphics?
        else if(vendor == clw::EPlatformVendor::Intel)
            return 1;
        else if(vendor == clw::EPlatformVendor::NVIDIA)
            return device().warpSize();

        return 1;
    }
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
