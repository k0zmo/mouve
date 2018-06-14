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
#include "AmdtActivityLogger.h"

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

// Set to one if you need to debug the kernels using OpenCL Intel runtime (only CPU)
#define INTEL_DEBUGGING 0

namespace {

boost::filesystem::path kernelsDirectory();
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
#if !(defined(_DEBUG) || defined(DEBUG) && !defined(NDEBUG))
    if(_interactiveInit)
        res = createInteractive();
    else
#endif
        res = createDefault();

    if(res)
    {
        // TODO Use filesystem::path inside library too
        _library.create(_context, kernelsDirectory().string() + "/");
        _logger = makeActivityLogger();
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
        if(error_id != CL_BUILD_PROGRAM_FAILURE && error_id != CL_INVALID_KERNEL_NAME)
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

namespace {

boost::filesystem::path kernelsDirectory()
{
    const auto dir = boost::dll::program_location().parent_path() / "kernels";
    return boost::filesystem::absolute(dir);
}
} // namespace

string GpuNodeModule::additionalBuildOptions(const std::string& programName) const
{
#if INTEL_DEBUGGING == 1
    // Enable kernel debugging if device is CPU and it's Intel platform
    if (_device.platform().vendorEnum() == clw::EPlatformVendor::Intel &&
        _device.deviceType() == clw::EDeviceType::Cpu)
    {
        const static auto dir = kernelsDirectory();
        return fmt::format(" -g -s \"{}\"",
                           boost::filesystem::absolute(dir / programName).string());
    }
#else
    (void)programName;
#endif
    return {};
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
