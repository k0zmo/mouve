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

#include "Logic/Nodes/CV.h"

class GpuBayerToGrayNodeType : public GpuNodeType
{
public:
    GpuBayerToGrayNodeType()
        : _redGain(1.0)
        , _greenGain(1.0)
        , _blueGain(1.0)
        , _BayerCode(cvu::EBayerCode::RG)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::BayerFormat:
            _BayerCode = newValue.toEnum().cast<cvu::EBayerCode>();
            return true;
        case pid::RedGain:
            _redGain = newValue.toDouble();
            return true;
        case pid::GreenGain:
            _greenGain = newValue.toDouble();
            return true;
        case pid::BlueGain:
            _blueGain = newValue.toDouble();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::BayerFormat: return _BayerCode;
        case pid::RedGain: return _redGain;
        case pid::GreenGain: return _greenGain;
        case pid::BlueGain: return _blueGain;
        }

        return NodeProperty();
    }

    bool postInit() override
    {
        // On AMD up from AMD APP 2.7 which supports 1.2 OpenCL we can use templates and other c++ goodies
        string opts;
        if(_gpuComputeModule->device().platform().vendorEnum() == clw::EPlatformVendor::AMD
            && _gpuComputeModule->device().platform().version() >= clw::EPlatformVersion::v1_2)
        {
            opts = "-DTEMPLATES_SUPPORTED -x clc++";
        }

        // Local version is a bit faster
        _kidConvertRG2Gray = _gpuComputeModule->registerKernel("convert_rg2gray_local", "bayer.cl", opts);
        _kidConvertGB2Gray = _gpuComputeModule->registerKernel("convert_gb2gray_local", "bayer.cl", opts);
        _kidConvertGR2Gray = _gpuComputeModule->registerKernel("convert_gr2gray_local", "bayer.cl", opts);
        _kidConvertBG2Gray = _gpuComputeModule->registerKernel("convert_bg2gray_local", "bayer.cl", opts);

        return _kidConvertRG2Gray != InvalidKernelID
            && _kidConvertGB2Gray != InvalidKernelID
            && _kidConvertGR2Gray != InvalidKernelID
            && _kidConvertBG2Gray != InvalidKernelID;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const clw::Image2D& input = reader.readSocket(0).getDeviceImageMono();
        clw::Image2D& output = writer.acquireSocket(0).getDeviceImageMono();

        int imageWidth = input.width();
        int imageHeight = input.height();

        if(imageWidth == 0 || imageHeight == 0)
            return ExecutionStatus(EStatus::Ok);

        clw::Kernel kernelConvertBayer2Gray;
        switch(_BayerCode)
        {
        case cvu::EBayerCode::RG:
            kernelConvertBayer2Gray = _gpuComputeModule->acquireKernel(_kidConvertRG2Gray);
            break;
        case cvu::EBayerCode::GB:
            kernelConvertBayer2Gray = _gpuComputeModule->acquireKernel(_kidConvertGB2Gray);
            break;
        case cvu::EBayerCode::GR:
            kernelConvertBayer2Gray = _gpuComputeModule->acquireKernel(_kidConvertGR2Gray);
            break;
        case cvu::EBayerCode::BG:
            kernelConvertBayer2Gray = _gpuComputeModule->acquireKernel(_kidConvertBG2Gray);
            break;
        }

        if(kernelConvertBayer2Gray.isNull())
            return ExecutionStatus(EStatus::Error, "Bad bayer code");

        // Ensure output image size is enough
        if(output.isNull() || output.width() != imageWidth || output.height() != imageHeight)
        {
            output = _gpuComputeModule->context().createImage2D(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device, 
                clw::ImageFormat(clw::EChannelOrder::R, clw::EChannelType::Normalized_UInt8), 
                imageWidth, imageHeight);
        }

        cl_float3 gains = { (float) _redGain, (float) _greenGain, (float) _blueGain };
        int sharedWidth = 16 + 2;
        int sharedHeight = 16 + 2;

        kernelConvertBayer2Gray.setLocalWorkSize(16, 16);
        kernelConvertBayer2Gray.setRoundedGlobalWorkSize(imageWidth, imageHeight);
        kernelConvertBayer2Gray.setArg(0, input);
        kernelConvertBayer2Gray.setArg(1, output);
        kernelConvertBayer2Gray.setArg(2, gains);
        kernelConvertBayer2Gray.setArg(3, clw::LocalMemorySize(sharedWidth*sharedHeight*sizeof(float)));
        kernelConvertBayer2Gray.setArg(4, sharedWidth);
        kernelConvertBayer2Gray.setArg(5, sharedHeight);

        _gpuComputeModule->queue().asyncRunKernel(kernelConvertBayer2Gray);
        _gpuComputeModule->queue().finish();

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::DeviceImageMono, "input", "Input", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::DeviceImageMono, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Enum, "Bayer format", "item: BG, item: GB, item: RG, item: GR" },
            { EPropertyType::Double, "Red gain", "min:0.0, max:4.0, step:0.01" },
            { EPropertyType::Double, "Green gain", "min:0.0, max:4.0, step:0.01" },
            { EPropertyType::Double, "Blue gain", "min:0.0, max:4.0, step:0.01" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Performs demosaicing from Bayer pattern image to RGB image";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
        nodeConfig.module = "opencl";
    }

private:
    enum class pid
    {
        BayerFormat,
        RedGain,
        GreenGain,
        BlueGain,
    };

    double _redGain;
    double _greenGain;
    double _blueGain;
    cvu::EBayerCode _BayerCode;

private:
    KernelID _kidConvertRG2Gray;
    KernelID _kidConvertGB2Gray;
    KernelID _kidConvertGR2Gray;
    KernelID _kidConvertBG2Gray;
};

class GpuBayerToRgbNodeType : public GpuNodeType
{
public:
    GpuBayerToRgbNodeType()
        : _redGain(1.0)
        , _greenGain(1.0)
        , _blueGain(1.0)
        , _BayerCode(cvu::EBayerCode::RG)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::BayerFormat:
            _BayerCode = newValue.toEnum().cast<cvu::EBayerCode>();
            return true;
        case pid::RedGain:
            _redGain = newValue.toDouble();
            return true;
        case pid::GreenGain:
            _greenGain = newValue.toDouble();
            return true;
        case pid::BlueGain:
            _blueGain = newValue.toDouble();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::BayerFormat: return _BayerCode;
        case pid::RedGain: return _redGain;
        case pid::GreenGain: return _greenGain;
        case pid::BlueGain: return _blueGain;
        }

        return NodeProperty();
    }

    bool postInit() override
    {
        // On AMD up from AMD APP 2.7 which supports 1.2 OpenCL we can use templates and other c++ goodies
        string opts;
        if(_gpuComputeModule->device().platform().vendorEnum() == clw::EPlatformVendor::AMD
            && _gpuComputeModule->device().platform().version() >= clw::EPlatformVersion::v1_2)
        {
            opts = "-DTEMPLATES_SUPPORTED -x clc++";
        }

        // Local version is a bit faster
        _kidConvertRG2Rgb = _gpuComputeModule->registerKernel("convert_rg2rgb_local", "bayer.cl", opts);
        _kidConvertGB2Rgb = _gpuComputeModule->registerKernel("convert_gb2rgb_local", "bayer.cl", opts);
        _kidConvertGR2Rgb = _gpuComputeModule->registerKernel("convert_gr2rgb_local", "bayer.cl", opts);
        _kidConvertBG2Rgb = _gpuComputeModule->registerKernel("convert_bg2rgb_local", "bayer.cl", opts);

        return _kidConvertRG2Rgb != InvalidKernelID
            && _kidConvertGB2Rgb != InvalidKernelID
            && _kidConvertGR2Rgb != InvalidKernelID
            && _kidConvertBG2Rgb != InvalidKernelID;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const clw::Image2D& input = reader.readSocket(0).getDeviceImageMono();
        clw::Image2D& output = writer.acquireSocket(0).getDeviceImageRgb();

        int imageWidth = input.width();
        int imageHeight = input.height();

        if(imageWidth == 0 || imageHeight == 0)
            return ExecutionStatus(EStatus::Ok);

        clw::Kernel kernelConvertBayer2Rgb;
        switch(_BayerCode)
        {
        case cvu::EBayerCode::RG:
            kernelConvertBayer2Rgb = _gpuComputeModule->acquireKernel(_kidConvertRG2Rgb);
            break;
        case cvu::EBayerCode::GB:
            kernelConvertBayer2Rgb = _gpuComputeModule->acquireKernel(_kidConvertGB2Rgb);
            break;
        case cvu::EBayerCode::GR:
            kernelConvertBayer2Rgb = _gpuComputeModule->acquireKernel(_kidConvertGR2Rgb);
            break;
        case cvu::EBayerCode::BG:
            kernelConvertBayer2Rgb = _gpuComputeModule->acquireKernel(_kidConvertBG2Rgb);
            break;
        }

        if(kernelConvertBayer2Rgb.isNull())
            return ExecutionStatus(EStatus::Error, "Bad bayer code");

        // Ensure output image size is enough
        if(output.isNull() || output.width() != imageWidth || output.height() != imageHeight)
        {
            output = _gpuComputeModule->context().createImage2D(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device, 
                clw::ImageFormat(clw::EChannelOrder::RGBA, clw::EChannelType::Normalized_UInt8), 
                imageWidth, imageHeight);
        }

        cl_float3 gains = { (float) _redGain, (float) _greenGain, (float) _blueGain };
        int sharedWidth = 16 + 2;
        int sharedHeight = 16 + 2;

        kernelConvertBayer2Rgb.setLocalWorkSize(16, 16);
        kernelConvertBayer2Rgb.setRoundedGlobalWorkSize(imageWidth, imageHeight);
        kernelConvertBayer2Rgb.setArg(0, input);
        kernelConvertBayer2Rgb.setArg(1, output);
        kernelConvertBayer2Rgb.setArg(2, gains);
        kernelConvertBayer2Rgb.setArg(3, clw::LocalMemorySize(sharedWidth*sharedHeight*sizeof(float)));
        kernelConvertBayer2Rgb.setArg(4, sharedWidth);
        kernelConvertBayer2Rgb.setArg(5, sharedHeight);

        _gpuComputeModule->queue().asyncRunKernel(kernelConvertBayer2Rgb);
        _gpuComputeModule->queue().finish();

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::DeviceImageMono, "input", "Input", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::DeviceImageRgb, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Enum, "Bayer format", "item: BG, item: GB, item: RG, item: GR" },
            { EPropertyType::Double, "Red gain", "min:0.0, max:4.0, step:0.01" },
            { EPropertyType::Double, "Green gain", "min:0.0, max:4.0, step:0.01" },
            { EPropertyType::Double, "Blue gain", "min:0.0, max:4.0, step:0.01" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Performs demosaicing from Bayer pattern image to RGB image";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
        nodeConfig.module = "opencl";
    }

private:
    enum class pid
    {
        BayerFormat,
        RedGain,
        GreenGain,
        BlueGain,
    };

    double _redGain;
    double _greenGain;
    double _blueGain;
    cvu::EBayerCode _BayerCode;

private:
    KernelID _kidConvertRG2Rgb;
    KernelID _kidConvertGB2Rgb;
    KernelID _kidConvertGR2Rgb;
    KernelID _kidConvertBG2Rgb;
};

REGISTER_NODE("OpenCL/Format conversion/Gray de-bayer", GpuBayerToGrayNodeType)
REGISTER_NODE("OpenCL/Format conversion/RGB de-bayer", GpuBayerToRgbNodeType)

#endif
