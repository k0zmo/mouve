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

#include "Logic/NodeSystem.h"
#include "Logic/OpenCL/GpuNode.h"

#include <fmt/core.h>

// Define as 1 to use more accurate calcuations
#define ACCURATE_CALCULATIONS 0

class GpuMixtureOfGaussiansNodeType : public GpuNodeType
{
public:
    GpuMixtureOfGaussiansNodeType()
        : _history(200),
          _nmixtures(5),
          _backgroundRatio(0.7f),
          _learningRate(-1),
          _showBackground(false),
          _nframe(0),
          _varianceThreshold(6.25f),
          _initialWeight(0.05f),
#if ACCURATE_CALCULATIONS != 1
          _initialVariance(15 * 15 * 4),
          _minVariance(15 * 15)
#else
          _initialVariance(500),
          _minVariance(0.4f)
#endif
    {
        addInput("Image", ENodeFlowDataType::DeviceImageMono);
        addOutput("Output", ENodeFlowDataType::DeviceImageMono);
        addOutput("Background", ENodeFlowDataType::DeviceImageMono);
        addProperty("History frames", _history)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 500))
            .setUiHints("min:1, max:500");
        addProperty("Number of mixtures", _nmixtures)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 9))
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty&) {
                std::string opts = fmt::format("-DNMIXTURES={} -DACCURATE_CALCULATIONS={}",
                                               _nmixtures, ACCURATE_CALCULATIONS);
                _kidGaussMix = _gpuComputeModule->registerKernel("mog_image_unorm", "mog.cl", opts);
                _kidGaussBackground =
                    _gpuComputeModule->registerKernel("mog_background_image_unorm", "mog.cl", opts);
            }))
            .setUiHints("min:1, max:9");
        addProperty("Background ratio", _backgroundRatio)
            .setValidator(make_validator<ExclRangePropertyValidator<double>>(0.0, 1.0))
            .setUiHints("min:0.01, max:0.99, step:0.01");
        addProperty("Learning rate", _learningRate)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(-1.0, 1.0))
            .setUiHints("min:-1, max:1, step:0.01, decimals:3");
        addProperty("Show background", _showBackground);
        setFlags(kl::make_flags(ENodeConfig::HasState));
        setModule("opencl");
    }

    bool postInit() override
    {
        std::string opts = fmt::format("-DNMIXTURES={} -DACCURATE_CALCULATIONS={}", _nmixtures,
                                       ACCURATE_CALCULATIONS);

        _kidGaussMix = _gpuComputeModule->registerKernel("mog_image_unorm", "mog.cl", opts);
        _kidGaussBackground =
            _gpuComputeModule->registerKernel("mog_background_image_unorm", "mog.cl", opts);
        return _kidGaussMix != InvalidKernelID && _kidGaussBackground != InvalidKernelID;
    }

    bool restart() override
    {
        _nframe = 0;

        // TUTAJ inicjalizuj bufory???
        // TODO: Uzyc asyncow

        if (!_mixtureDataBuffer.isNull())
        {
            // Wyzerowanie
            void* ptr =
                _gpuComputeModule->queue().mapBuffer(_mixtureDataBuffer, clw::EMapAccess::Write);
            memset(ptr, 0, _mixtureDataBuffer.size());
            _gpuComputeModule->queue().asyncUnmap(_mixtureDataBuffer, ptr);
        }

        // Parametry stale dla kernela
        struct MogParams
        {
            float varThreshold;
            float backgroundRatio;
            float w0;     // waga dla nowej mikstury
            float var0;   // wariancja dla nowej mikstury
            float minVar; // dolny prog mozliwej wariancji
        };

        // Create mixture parameter buffer
        if (_mixtureParamsBuffer.isNull())
        {
            _mixtureParamsBuffer = _gpuComputeModule->context().createBuffer(
                clw::EAccess::ReadOnly, clw::EMemoryLocation::Device, sizeof(MogParams));
        }

        MogParams* ptr = (MogParams*)_gpuComputeModule->queue().mapBuffer(_mixtureParamsBuffer,
                                                                          clw::EMapAccess::Write);
        ptr->varThreshold = _varianceThreshold;
        ptr->backgroundRatio = _backgroundRatio;
        ptr->w0 = _initialWeight;
        ptr->var0 = _initialVariance;
        ptr->minVar = _minVariance;
        _gpuComputeModule->queue().asyncUnmap(_mixtureParamsBuffer, ptr);

        return true;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const clw::Image2D& deviceImage = reader.readSocket(0).getDeviceImageMono();
        clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImageMono();

        int srcWidth = deviceImage.width();
        int srcHeight = deviceImage.height();

        if (srcWidth == 0 || srcHeight == 0)
            return ExecutionStatus(EStatus::Ok);

        clw::Kernel kernelGaussMix = _gpuComputeModule->acquireKernel(_kidGaussMix);

        /*
            Create mixture data buffer
        */
        resetMixturesState(srcWidth * srcHeight);

        if (deviceDest.isNull() || deviceDest.width() != srcWidth ||
            deviceDest.height() != srcHeight)
        {
            // Obraz (w zasadzie maska) pierwszego planu
            deviceDest = _gpuComputeModule->context().createImage2D(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
                clw::ImageFormat(clw::EChannelOrder::R, clw::EChannelType::Normalized_UInt8),
                srcWidth, srcHeight);
        }

        // Calculate dynamic learning rate (if necessary)
        ++_nframe;
        float alpha = _learningRate >= 0 && _nframe > 1
                          ? _learningRate
                          : 1.0f / (std::min)(_nframe, _history.cast<int>());

        kernelGaussMix.setLocalWorkSize(16, 16);
        kernelGaussMix.setRoundedGlobalWorkSize(srcWidth, srcHeight);
        kernelGaussMix.setArg(0, deviceImage);
        kernelGaussMix.setArg(1, deviceDest);
        kernelGaussMix.setArg(2, _mixtureDataBuffer);
        kernelGaussMix.setArg(3, _mixtureParamsBuffer);
        kernelGaussMix.setArg(4, alpha);
        _gpuComputeModule->queue().asyncRunKernel(kernelGaussMix);

        if (_showBackground)
        {
            clw::Image2D& deviceDestBackground = writer.acquireSocket(1).getDeviceImageMono();
            if (deviceDestBackground.isNull() || deviceDestBackground.width() != srcWidth ||
                deviceDestBackground.height() != srcHeight)
            {
                // Obraz (w zasadzie maska) pierwszego planu
                deviceDestBackground = _gpuComputeModule->context().createImage2D(
                    clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
                    clw::ImageFormat(clw::EChannelOrder::R, clw::EChannelType::Normalized_UInt8),
                    srcWidth, srcHeight);
            }

            clw::Kernel kernelBackground = _gpuComputeModule->acquireKernel(_kidGaussBackground);

            kernelBackground.setLocalWorkSize(16, 16);
            kernelBackground.setRoundedGlobalWorkSize(srcWidth, srcHeight);
            kernelBackground.setArg(0, deviceDestBackground);
            kernelBackground.setArg(1, _mixtureDataBuffer);
            kernelBackground.setArg(2, _mixtureParamsBuffer);
            _gpuComputeModule->queue().asyncRunKernel(kernelBackground);
        }

        _gpuComputeModule->queue().finish();
        return ExecutionStatus(EStatus::Ok);
    }

private:
    void resetMixturesState(int pixNumbers)
    {
        // Dane mikstur (stan wewnetrzny estymatora tla)
        const size_t mixtureDataSize = _nmixtures * pixNumbers * 3 * sizeof(float);

        if (_mixtureDataBuffer.isNull() || _mixtureDataBuffer.size() != mixtureDataSize)
        {
            _mixtureDataBuffer = _gpuComputeModule->context().createBuffer(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device, mixtureDataSize);

            // Wyzerowanie
            void* ptr =
                _gpuComputeModule->queue().mapBuffer(_mixtureDataBuffer, clw::EMapAccess::Write);
            memset(ptr, 0, mixtureDataSize);
            _gpuComputeModule->queue().unmap(_mixtureDataBuffer, ptr);
        }
    }

private:
    TypedNodeProperty<int> _history;
    TypedNodeProperty<int> _nmixtures;
    TypedNodeProperty<float> _backgroundRatio;
    TypedNodeProperty<float> _learningRate;
    TypedNodeProperty<bool> _showBackground;

    clw::Buffer _mixtureDataBuffer;
    clw::Buffer _mixtureParamsBuffer;
    KernelID _kidGaussMix;
    KernelID _kidGaussBackground;

    int _nframe;
    float _varianceThreshold;
    float _initialWeight;
    float _initialVariance;
    float _minVariance;
};

void registerGpuMOG(NodeSystem& system)
{
    system.registerNodeType("OpenCL/Video segmentation/Mixture of Gaussians",
                            makeDefaultNodeFactory<GpuMixtureOfGaussiansNodeType>());
}
