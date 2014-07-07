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

#include "IGpuNodeModule.h"
#include "GpuKernelLibrary.h"
#include "GpuActivityLogger.h"

using std::vector;
using std::string;

class LOGIC_EXPORT GpuNodeModule : public IGpuNodeModule
{
public:
    explicit GpuNodeModule(bool interactiveInit);
    ~GpuNodeModule() override;

    void setInteractiveInit(bool interactiveInit) override;
    bool isInteractiveInit() const override;

    bool initialize() override;
    bool isInitialized() override;
    std::string moduleName() const override;

    bool createDefault() override;
    bool createInteractive() override;

    vector<GpuPlatform> availablePlatforms() const override;

    KernelID registerKernel(const string& kernelName, 
        const string& programName, const string& buildOptions = "");
    clw::Kernel acquireKernel(KernelID kernelId);
    KernelID updateKernel(KernelID kernelId, const string& buildOptions);

    vector<GpuRegisteredProgram> populateListOfRegisteredPrograms() const override;
    void rebuildProgram(const string& programName) override;

    bool isConstantMemorySufficient(uint64_t memSize) const;
    bool isLocalMemorySufficient(uint64_t memSize) const;
    size_t warpSize() const;

    clw::Context& context();
    const clw::Context& context() const;
    clw::Device& device();
    const clw::Device& device() const;
    clw::CommandQueue& queue();
    const clw::CommandQueue& queue() const;
    clw::CommandQueue& dataQueue();
    const clw::CommandQueue& dataQueue() const;

    const GpuActivityLogger& activityLogger() const;

private:
    std::string additionalBuildOptions(const std::string& programName) const;
    bool createAfterContext();

private:
    clw::Context _context;
    clw::Device _device;
    clw::CommandQueue _queue;
    clw::CommandQueue _dataQueue;

    uint64_t _maxConstantMemory;
    uint64_t _maxLocalMemory;

    KernelLibrary _library;
    GpuActivityLogger _logger;

    bool _interactiveInit;
};

inline void GpuNodeModule::setInteractiveInit(bool interactiveInit)
{ _interactiveInit = interactiveInit; }
inline bool GpuNodeModule::isInteractiveInit() const
{ return _interactiveInit; }
inline bool GpuNodeModule::isConstantMemorySufficient(uint64_t memSize) const
{ return memSize <= _maxConstantMemory; }
inline bool GpuNodeModule::isLocalMemorySufficient(uint64_t memSize) const
{ return memSize <= _maxLocalMemory; }
inline clw::Context& GpuNodeModule::context()
{ return _context; }
inline const clw::Context& GpuNodeModule::context() const
{ return _context; }
inline clw::Device& GpuNodeModule::device()
{ return _device; }
inline const clw::Device& GpuNodeModule::device() const
{ return _device; }
inline clw::CommandQueue& GpuNodeModule::queue()
{ return _queue; }
inline const clw::CommandQueue& GpuNodeModule::queue() const
{ return _queue; }
inline clw::CommandQueue& GpuNodeModule::dataQueue()
{ return _dataQueue; }
inline const clw::CommandQueue& GpuNodeModule::dataQueue() const
{ return _dataQueue; }
inline const GpuActivityLogger& GpuNodeModule::activityLogger() const
{ return _logger; }

#endif