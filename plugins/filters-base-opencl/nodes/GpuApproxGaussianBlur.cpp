/*
 * Copyright (c) 2013-2018 Kajetan Swierk <k0zmo@outlook.com>
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

#include "Logic/OpenCL/GpuNode.h"
#include "Logic/NodeFactory.h"

#include <fmt/format.h>

class GpuApproxGaussianBlurNodeType : public GpuNodeType
{
public:
    GpuApproxGaussianBlurNodeType() : _sigma(1.2), _numPasses(3)
    {
        addInput("Input", ENodeFlowDataType::DeviceImageMono);
        addOutput("Output", ENodeFlowDataType::DeviceImageMono);
        addProperty("Sigma", _sigma)
            .setValidator(make_validator<GreaterPropertyValidator<double>>(0.0))
            .setUiHints("min:0.1, step:0.1");
        addProperty("Approx. passes", _numPasses)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 8))
            .setUiHints("min:1, max:8");
        setModule("opencl");
    }

    bool postInit() override
    {
        _kidApproxGaussianBlurHoriz = _gpuComputeModule->registerKernel(
            "approxGaussian_horiz_image", "approx_gaussian.cl", kernelBuildOptionsHoriz(512));
        _kidApproxGaussianBlurVert = _gpuComputeModule->registerKernel(
            "approxGaussian_vert_image", "approx_gaussian.cl", kernelBuildOptionsVert(512));

        return _kidApproxGaussianBlurHoriz != InvalidKernelID &&
               _kidApproxGaussianBlurVert != InvalidKernelID;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const clw::Image2D& input = reader.readSocket(0).getDeviceImageMono();
        clw::Image2D& output = writer.acquireSocket(0).getDeviceImageMono();

        int imageWidth = input.width();
        int imageHeight = input.height();

        if (imageWidth == 0 || imageHeight == 0)
            return ExecutionStatus(EStatus::Ok);

        // Ensure temporary image size is enough
        if (_tempImage_cl.isNull() || _tempImage_cl.width() != imageWidth ||
            _tempImage_cl.height() != imageHeight)
        {
            _tempImage_cl = _gpuComputeModule->context().createImage2D(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
                clw::ImageFormat(clw::EChannelOrder::R, clw::EChannelType::Float), imageWidth,
                imageHeight);

            // _tempImage's size is kept in sync with defines for both of kernels
            _kidApproxGaussianBlurHoriz = _gpuComputeModule->registerKernel(
                "approxGaussian_horiz_image", "approx_gaussian.cl",
                kernelBuildOptionsHoriz(imageWidth));
            _kidApproxGaussianBlurVert =
                _gpuComputeModule->registerKernel("approxGaussian_vert_image", "approx_gaussian.cl",
                                                  kernelBuildOptionsVert(imageHeight));
        }

        // Ensure output image size is enough
        if (output.isNull() || output.width() != imageWidth || output.height() != imageHeight)
        {
            output = _gpuComputeModule->context().createImage2D(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
                clw::ImageFormat(clw::EChannelOrder::R, clw::EChannelType::Normalized_UInt8),
                imageWidth, imageHeight);
        }

        clw::Kernel kernelApproxGaussianBlurHoriz =
            _gpuComputeModule->acquireKernel(_kidApproxGaussianBlurHoriz);
        clw::Kernel kernelApproxGaussianBlurVert =
            _gpuComputeModule->acquireKernel(_kidApproxGaussianBlurVert);

        cl_uint numApproxPasses = _numPasses - 1;
        cl_float boxWidth = calculateBoxFilterWidth(static_cast<float>(_sigma), _numPasses);
        cl_float halfBoxWidth = boxWidth * 0.5f;
        cl_float fracHalfBoxWidth = (halfBoxWidth + 0.5f) - (int)(halfBoxWidth + 0.5f);
        cl_float invFracHalfBoxWidth = 1.0f - fracHalfBoxWidth;
        cl_float rcpBoxWidth = 1.0f / boxWidth;

        kernelApproxGaussianBlurHoriz.setLocalWorkSize(threadsPerGroup, 1);
        kernelApproxGaussianBlurHoriz.setGlobalWorkSize(threadsPerGroup, imageHeight);
        kernelApproxGaussianBlurHoriz.setArg(0, input);
        kernelApproxGaussianBlurHoriz.setArg(1, _tempImage_cl);
        // We need at least threadsPerGroup * 2 shared memory: in warp scan phase (scanWarpInclusive
        // function) each thread in a warp/wavefront addresses two shared memory cells.
        kernelApproxGaussianBlurHoriz.setArg(
            2, clw::TypedLocalMemorySize<float>((std::max)(imageWidth, threadsPerGroup * 2)));
        kernelApproxGaussianBlurHoriz.setArg(3, numApproxPasses);
        kernelApproxGaussianBlurHoriz.setArg(4, halfBoxWidth);
        kernelApproxGaussianBlurHoriz.setArg(5, fracHalfBoxWidth);
        kernelApproxGaussianBlurHoriz.setArg(6, invFracHalfBoxWidth);
        kernelApproxGaussianBlurHoriz.setArg(7, rcpBoxWidth);
        _gpuComputeModule->queue().asyncRunKernel(kernelApproxGaussianBlurHoriz);

        kernelApproxGaussianBlurVert.setLocalWorkSize(threadsPerGroup, 1);
        kernelApproxGaussianBlurVert.setGlobalWorkSize(threadsPerGroup, imageWidth);
        kernelApproxGaussianBlurVert.setArg(0, _tempImage_cl);
        kernelApproxGaussianBlurVert.setArg(1, output);
        kernelApproxGaussianBlurVert.setArg(
            2, clw::TypedLocalMemorySize<float>((std::max)(imageHeight, threadsPerGroup * 2)));
        kernelApproxGaussianBlurVert.setArg(3, numApproxPasses);
        kernelApproxGaussianBlurVert.setArg(4, halfBoxWidth);
        kernelApproxGaussianBlurVert.setArg(5, fracHalfBoxWidth);
        kernelApproxGaussianBlurVert.setArg(6, invFracHalfBoxWidth);
        kernelApproxGaussianBlurVert.setArg(7, rcpBoxWidth);
        _gpuComputeModule->queue().runKernel(kernelApproxGaussianBlurVert);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    float calculateBoxFilterWidth(float sigma, int numPasses)
    {
        return sqrt(sigma * sigma * 12.0f / numPasses + 1);
    }

    fmt::memory_buffer kernelBuildOptionsBase()
    {
        const size_t warpSize = _gpuComputeModule->warpSize();
        const size_t warpSizeLog2 = static_cast<size_t>(log2(static_cast<double>(warpSize)));
        fmt::memory_buffer buf;
        fmt::format_to(buf, "-DWARP_SIZE={} -DWARP_SIZE_LOG_2={} -DNUM_THREADS={} -DNUM_WARPS={}",
                       warpSize, warpSizeLog2, threadsPerGroup, threadsPerGroup / warpSize);
        return buf;
    }

    std::string kernelBuildOptionsHoriz(int imageWidth)
    {
        fmt::memory_buffer buf = kernelBuildOptionsBase();
        fmt::format_to(buf, " -DIMAGE_WIDTH={} -DTEXELS_PER_THREAD={} -DAPPROX_GAUSSIAN_PASS=0",
                       imageWidth, (imageWidth + threadsPerGroup - 1) / threadsPerGroup);
        return to_string(buf);
    }

    std::string kernelBuildOptionsVert(int imageHeight)
    {
        fmt::memory_buffer buf = kernelBuildOptionsBase();
        fmt::format_to(buf, " -DIMAGE_HEIGHT={} -DTEXELS_PER_THREAD={} -DAPPROX_GAUSSIAN_PASS=1",
                       imageHeight, (imageHeight + threadsPerGroup - 1) / threadsPerGroup);
        return to_string(buf);
    }

private:
    TypedNodeProperty<double> _sigma;
    TypedNodeProperty<int> _numPasses;

    static const int threadsPerGroup = 256;

private:
    KernelID _kidApproxGaussianBlurHoriz;
    KernelID _kidApproxGaussianBlurVert;

    clw::Image2D _tempImage_cl;
};

REGISTER_NODE("OpenCL/Filters/Approx. Gaussian blur", GpuApproxGaussianBlurNodeType)
