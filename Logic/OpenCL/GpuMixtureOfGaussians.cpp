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

#include "GpuNode.h"
#include "Logic/NodeFactory.h"
#include "Kommon/StringUtils.h"

// Define as 1 to use more accurate calcuations
#define ACCURATE_CALCULATIONS 0

class GpuMixtureOfGaussiansNodeType : public GpuNodeType
{
public:
    GpuMixtureOfGaussiansNodeType()
        : _nmixtures(5)
        , _nframe(0)
        , _history(200)
        , _learningRate(-1)
        , _varianceThreshold(6.25f)
        , _backgroundRatio(0.7f)
        , _initialWeight(0.05f)
#if ACCURATE_CALCULATIONS != 1
        , _initialVariance(15*15*4)
        , _minVariance(15*15)
#else
        , _initialVariance(500)
        , _minVariance(0.4f)
#endif
        , _showBackground(false)
    {
    }

    bool postInit() override
    {
        std::string opts = string_format("-DNMIXTURES=%d -DACCURATE_CALCULATIONS=%d",
            _nmixtures, ACCURATE_CALCULATIONS);

        _kidGaussMix = _gpuComputeModule->registerKernel(
            "mog_image_unorm", "mog.cl", opts);
        _kidGaussBackground = _gpuComputeModule->registerKernel(
            "mog_background_image_unorm", "mog.cl", opts);
        return _kidGaussMix != InvalidKernelID &&
            _kidGaussBackground != InvalidKernelID;
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::History:
            _history = newValue.toInt();
            return true;
        case pid::NMixtures:
            {
                _nmixtures = newValue.toInt();

                std::string opts = string_format("-DNMIXTURES=%d -DACCURATE_CALCULATIONS=%d",
                    _nmixtures, ACCURATE_CALCULATIONS);

                _kidGaussMix = _gpuComputeModule->registerKernel(
                    "mog_image_unorm", "mog.cl", opts);
                _kidGaussBackground = _gpuComputeModule->registerKernel(
                    "mog_background_image_unorm", "mog.cl", opts);

                return true;
            }
        case pid::BackgroundRatio:
            _backgroundRatio = newValue.toFloat();
            return true;
        case pid::LearningRate:
            _learningRate = newValue.toFloat();
            return true;
        case pid::ShowBackground:
            _showBackground = newValue.toBool();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::History: return _history;
        case pid::NMixtures: return _nmixtures;
        case pid::BackgroundRatio: return _backgroundRatio;
        case pid::LearningRate: return _learningRate;
        case pid::ShowBackground: return _showBackground;
        }

        return NodeProperty();
    }

    bool restart() override
    {
        _nframe = 0;

        // TUTAJ inicjalizuj bufory???
        // TODO: Uzyc asyncow

        if(!_mixtureDataBuffer.isNull())
        {
            // Wyzerowanie
            void* ptr = _gpuComputeModule->queue().mapBuffer(_mixtureDataBuffer, clw::EMapAccess::Write);
            memset(ptr, 0, _mixtureDataBuffer.size());
            _gpuComputeModule->queue().asyncUnmap(_mixtureDataBuffer, ptr);
        }

        // Parametry stale dla kernela
        struct MogParams
        {
            float varThreshold;
            float backgroundRatio;
            float w0; // waga dla nowej mikstury
            float var0; // wariancja dla nowej mikstury
            float minVar; // dolny prog mozliwej wariancji
        };

        // Create mixture parameter buffer
        if(_mixtureParamsBuffer.isNull())
        {
            _mixtureParamsBuffer = _gpuComputeModule->context().createBuffer(
                clw::EAccess::ReadOnly, clw::EMemoryLocation::Device, sizeof(MogParams));
        }

        MogParams* ptr = (MogParams*) _gpuComputeModule->queue().mapBuffer(
            _mixtureParamsBuffer, clw::EMapAccess::Write);
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

        if(srcWidth == 0 || srcHeight == 0)
            return ExecutionStatus(EStatus::Ok);

        clw::Kernel kernelGaussMix = _gpuComputeModule->acquireKernel(_kidGaussMix);

        /*
            Create mixture data buffer
        */
        resetMixturesState(srcWidth * srcHeight);

        if(deviceDest.isNull()
            || deviceDest.width() != srcWidth
            || deviceDest.height() != srcHeight)
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
            : 1.0f/std::min(_nframe, _history);

        kernelGaussMix.setLocalWorkSize(16, 16);
        kernelGaussMix.setRoundedGlobalWorkSize(srcWidth, srcHeight);
        kernelGaussMix.setArg(0, deviceImage);
        kernelGaussMix.setArg(1, deviceDest);
        kernelGaussMix.setArg(2, _mixtureDataBuffer);
        kernelGaussMix.setArg(3, _mixtureParamsBuffer);
        kernelGaussMix.setArg(4, alpha);
        _gpuComputeModule->queue().asyncRunKernel(kernelGaussMix);

        if(_showBackground)
        {
            clw::Image2D& deviceDestBackground = writer.acquireSocket(1).getDeviceImageMono();
            if(deviceDestBackground.isNull()
                || deviceDestBackground.width() != srcWidth
                || deviceDestBackground.height() != srcHeight)
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

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::DeviceImageMono, "input", "Image", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::DeviceImageMono, "output", "Output", "" },
            { ENodeFlowDataType::DeviceImageMono, "background", "Background", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Integer, "History frames", "min:1, max:500" },
            { EPropertyType::Integer, "Number of mixtures",  "min:1, max:9" },
            { EPropertyType::Double, "Background ratio", "min:0.01, max:0.99, step:0.01" },
            { EPropertyType::Double, "Learning rate", "min:-1, max:1, step:0.01, decimals:3" },
            { EPropertyType::Boolean, "Show background", "" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
        nodeConfig.flags = ENodeConfig::HasState;
        nodeConfig.module = "opencl";
    }

private:
    void resetMixturesState(int pixNumbers)
    {
        // Dane mikstur (stan wewnetrzny estymatora tla)
        const size_t mixtureDataSize = _nmixtures * pixNumbers * 3 * sizeof(float);

        if(_mixtureDataBuffer.isNull()
            || _mixtureDataBuffer.size() != mixtureDataSize)
        {
            _mixtureDataBuffer = _gpuComputeModule->context().createBuffer(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device, mixtureDataSize);

            // Wyzerowanie
            void* ptr = _gpuComputeModule->queue().mapBuffer(_mixtureDataBuffer, clw::EMapAccess::Write);
            memset(ptr, 0, mixtureDataSize);
            _gpuComputeModule->queue().unmap(_mixtureDataBuffer, ptr);
        }
    }

private:
    enum class pid
    {
        History,
        NMixtures,
        BackgroundRatio,
        LearningRate,
        ShowBackground
    };

    clw::Buffer _mixtureDataBuffer;
    clw::Buffer _mixtureParamsBuffer;
    KernelID _kidGaussMix;
    KernelID _kidGaussBackground;

    int _nmixtures;
    int _nframe;
    int _history;
    float _learningRate;
    float _varianceThreshold;
    float _backgroundRatio;
    float _initialWeight;
    float _initialVariance;
    float _minVariance;
    bool _showBackground;
};

REGISTER_NODE("OpenCL/Video segmentation/Mixture of Gaussians", GpuMixtureOfGaussiansNodeType)

#endif
