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

#include "Kommon/StringUtils.h"
#include "Logic/NodePlugin.h"
#include "Logic/NodeSystem.h"
#include "Logic/OpenCL/GpuNode.h"

#include "cvu.h"

class GpuKuwaharaFilterNodeType : public GpuNodeType
{
public:
    GpuKuwaharaFilterNodeType()
        : _radius(2)
    {
    }

    bool postInit() override
    {
        _kidKuwahara = _gpuComputeModule->registerKernel("kuwahara", "kuwahara.cl", "-DN_SECTORS=0");
        _kidKuwaharaRgb = _gpuComputeModule->registerKernel("kuwaharaRgb", "kuwahara.cl", "-DN_SECTORS=0");
        return _kidKuwahara != InvalidKernelID
            && _kidKuwaharaRgb != InvalidKernelID;
    }
    
    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Radius:
            _radius = newValue.toInt();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Radius: return _radius;
        }

        return NodeProperty();
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // inputs
        const clw::Image2D& deviceSrc = reader.readSocket(0).getDeviceImage();
        // outputs
        clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImage();

        // validate inputs
        if(deviceSrc.width() == 0 || deviceSrc.height() == 0)
            return ExecutionStatus(EStatus::Ok);

        int width = deviceSrc.width();
        int height = deviceSrc.height();
        clw::ImageFormat format = deviceSrc.format();

        // Prepare destination image and structuring element on a device
        ensureSizeIsEnough(deviceDest, width, height, format);

        KernelID kid = format.order == clw::EChannelOrder::R ? _kidKuwahara : _kidKuwaharaRgb;
        clw::Kernel kernelKuwahara = _gpuComputeModule->acquireKernel(kid);
        kernelKuwahara.setLocalWorkSize(clw::Grid(16, 16));
        kernelKuwahara.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelKuwahara.setArg(0, deviceSrc);
        kernelKuwahara.setArg(1, deviceDest);
        kernelKuwahara.setArg(2, _radius);
        _gpuComputeModule->queue().runKernel(kernelKuwahara);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::DeviceImage, "source", "Source", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::DeviceImage, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Integer, "Radius", "min: 1, max:15" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Kuwahara filter";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
        nodeConfig.module = "opencl";
    }

private:
    void ensureSizeIsEnough(clw::Image2D& image, int width,
        int height, const clw::ImageFormat& format)
    {
        if(image.isNull() || 
            image.width() != width || 
            image.height() != height ||
            image.format() != format)
        {
            image = _gpuComputeModule->context().createImage2D(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
                format, width, height);
        }
    }

private:
    enum class pid
    {
        Radius,
    };

    int _radius;
    KernelID _kidKuwahara;
    KernelID _kidKuwaharaRgb;
};

class GpuGeneralizedKuwaharaFilterNodeType : public GpuNodeType
{
public:
    GpuGeneralizedKuwaharaFilterNodeType()
        : _radius(6)
        , _N(8)
        , _smoothing(0.3333f)
        , _recreateKernelImage(true)
    {
    }

    bool postInit() override
    {
        string opts = string_format("-DN_SECTORS=%d", _N);

        _kidGeneralizedKuwahara = _gpuComputeModule->registerKernel("generalizedKuwahara", "kuwahara.cl", opts);
        _kidGeneralizedKuwaharaRgb = _gpuComputeModule->registerKernel("generalizedKuwaharaRgb", "kuwahara.cl", opts);
        return _kidGeneralizedKuwahara != InvalidKernelID
            && _kidGeneralizedKuwaharaRgb != InvalidKernelID;
    }
    
    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Radius:
            _radius = newValue.toInt();
            return true;
        case pid::N:
            _N = newValue.toInt();
            _recreateKernelImage = true;
            postInit();
            return true;
        case pid::Smoothing:
            _smoothing = newValue.toFloat();
            _recreateKernelImage = true;
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Radius: return _radius;
        case pid::N: return _N;
        case pid::Smoothing: return _smoothing;
        }

        return NodeProperty();
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // inputs
        const clw::Image2D& deviceSrc = reader.readSocket(0).getDeviceImage();
        // outputs
        clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImage();

        // validate inputs
        if(deviceSrc.width() == 0 || deviceSrc.height() == 0)
            return ExecutionStatus(EStatus::Ok);

        int width = deviceSrc.width();
        int height = deviceSrc.height();
        clw::ImageFormat format = deviceSrc.format();

        // Prepare destination image and structuring element on a device
        ensureSizeIsEnough(deviceDest, width, height, format);

        if(_recreateKernelImage)
        {
            cv::Mat kernel_;
            cvu::getGeneralizedKuwaharaKernel(kernel_, _N, _smoothing);
            _kernelImage = _gpuComputeModule->context().createImage2D(clw::EAccess::ReadOnly,
                clw::EMemoryLocation::Device, clw::ImageFormat(clw::EChannelOrder::R, clw::EChannelType::Float),
                kernel_.cols, kernel_.rows, kernel_.data);
            _recreateKernelImage = false;
        }

        KernelID kid = format.order == clw::EChannelOrder::R ? 
            _kidGeneralizedKuwahara : _kidGeneralizedKuwaharaRgb;
        clw::Kernel kernelKuwahara = _gpuComputeModule->acquireKernel(kid);
        kernelKuwahara.setLocalWorkSize(clw::Grid(16, 16));
        kernelKuwahara.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelKuwahara.setArg(0, deviceSrc);
        kernelKuwahara.setArg(1, deviceDest);
        kernelKuwahara.setArg(2, _radius);
        kernelKuwahara.setArg(3, _kernelImage);
        _gpuComputeModule->queue().runKernel(kernelKuwahara);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::DeviceImage, "source", "Source", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::DeviceImage, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Integer, "Radius", "min: 1, max:20" },
            { EPropertyType::Integer, "N", "min: 3, max:8" },
            { EPropertyType::Double, "Smoothing", "min:0.00, max:1.00" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Generalized Kuwahara filter";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
        nodeConfig.module = "opencl";
    }

private:
    void ensureSizeIsEnough(clw::Image2D& image, int width,
        int height, const clw::ImageFormat& format)
    {
        if(image.isNull() || 
            image.width() != width || 
            image.height() != height ||
            image.format() != format)
        {
            image = _gpuComputeModule->context().createImage2D(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
                format, width, height);
        }
    }

private:
    enum class pid
    {
        Radius,
        N,
        Smoothing
    };

    int _radius;
    int _N;
    float _smoothing;

    KernelID _kidGeneralizedKuwahara;
    KernelID _kidGeneralizedKuwaharaRgb;

    clw::Image2D _kernelImage;
    bool _recreateKernelImage;
};

class GpuAnisotropicKuwaharaFilterNodeType : public GpuNodeType
{
public:
    GpuAnisotropicKuwaharaFilterNodeType()
        : _radius(6)
        , _N(8)
        , _smoothing(0.3333f)
        , _recreateKernelImage(true)
    {
    }

    bool postInit() override
    {
        string opts = string_format("-DN_SECTORS=%d", _N);

        _kidAnisotropicKuwahara = _gpuComputeModule->registerKernel("anisotropicKuwahara", "kuwahara.cl", opts);
        _kidAnisotropicKuwaharaRgb = _gpuComputeModule->registerKernel("anisotropicKuwaharaRgb", "kuwahara.cl", opts);
        _kidCalcStructureTensor = _gpuComputeModule->registerKernel("calcStructureTensor", "kuwahara.cl", opts);
        _kidCalcStructureTensorRgb = _gpuComputeModule->registerKernel("calcStructureTensorRgb", "kuwahara.cl", opts);
        _kidConvolutionGaussian = _gpuComputeModule->registerKernel("convolutionGaussian", "kuwahara.cl", opts);
        _kidCalcOrientationAndAnisotropy = _gpuComputeModule->registerKernel("calcOrientationAndAnisotropy", "kuwahara.cl", opts);

        return _kidAnisotropicKuwahara != InvalidKernelID
            && _kidAnisotropicKuwaharaRgb != InvalidKernelID
            && _kidCalcStructureTensor != InvalidKernelID
            && _kidCalcStructureTensorRgb != InvalidKernelID
            && _kidConvolutionGaussian != InvalidKernelID
            && _kidCalcOrientationAndAnisotropy != InvalidKernelID;
    }
    
    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Radius:
            _radius = newValue.toInt();
            return true;
        case pid::N:
            _N = newValue.toInt();
            _recreateKernelImage = true;
            postInit();
            return true;
        case pid::Smoothing:
            _smoothing = newValue.toFloat();
            _recreateKernelImage = true;
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Radius: return _radius;
        case pid::N: return _N;
        case pid::Smoothing: return _smoothing;
        }

        return NodeProperty();
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // inputs
        const clw::Image2D& deviceSrc = reader.readSocket(0).getDeviceImage();
        // outputs
        clw::Image2D& deviceDest = writer.acquireSocket(0).getDeviceImage();

        // validate inputs
        if(deviceSrc.width() == 0 || deviceSrc.height() == 0)
            return ExecutionStatus(EStatus::Ok);

        int width = deviceSrc.width();
        int height = deviceSrc.height();
        clw::ImageFormat format = deviceSrc.format();

        // Prepare destination image and structuring element on a device
        ensureSizeIsEnough(deviceDest, width, height, format);
        ensureSizeIsEnough(_secondMomentMatrix, width, height,
            clw::ImageFormat(clw::EChannelOrder::RGBA, clw::EChannelType::Float));
        ensureSizeIsEnough(_secondMomentMatrix2, width, height,
            clw::ImageFormat(clw::EChannelOrder::RGBA, clw::EChannelType::Float));
        ensureSizeIsEnough(_oriAni, width, height,
            clw::ImageFormat(clw::EChannelOrder::RG, clw::EChannelType::Float));

        if(_recreateKernelImage)
        {
            cv::Mat kernel_;
            cvu::getGeneralizedKuwaharaKernel(kernel_, _N, _smoothing);
            _kernelImage = _gpuComputeModule->context().createImage2D(clw::EAccess::ReadOnly,
                clw::EMemoryLocation::Device, clw::ImageFormat(clw::EChannelOrder::R, clw::EChannelType::Float),
                kernel_.cols, kernel_.rows, kernel_.data);
            _recreateKernelImage = false;
        }

        clw::Kernel kernelCalcStructureTensor = _gpuComputeModule->acquireKernel(
            format.order == clw::EChannelOrder::R ? _kidCalcStructureTensor : _kidCalcStructureTensorRgb);
        kernelCalcStructureTensor.setLocalWorkSize(clw::Grid(16, 16));
        kernelCalcStructureTensor.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelCalcStructureTensor.setArg(0, deviceSrc);
        kernelCalcStructureTensor.setArg(1, _secondMomentMatrix);
        _gpuComputeModule->queue().asyncRunKernel(kernelCalcStructureTensor);

        // const float sigmaSst = 2.0f;
        // cv::Mat gaussianKernel = cv::getGaussianKernel(11, sigmaSst, CV_32F);
        clw::Kernel kernelConvolutionGaussian = _gpuComputeModule->acquireKernel(_kidConvolutionGaussian);
        kernelConvolutionGaussian.setLocalWorkSize(clw::Grid(16, 16));
        kernelConvolutionGaussian.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelConvolutionGaussian.setArg(0, _secondMomentMatrix);
        kernelConvolutionGaussian.setArg(1, _secondMomentMatrix2);
        _gpuComputeModule->queue().asyncRunKernel(kernelConvolutionGaussian);

        clw::Kernel kernelCalcOrientationAndAnisotropy = _gpuComputeModule->acquireKernel(_kidCalcOrientationAndAnisotropy);
        kernelCalcOrientationAndAnisotropy.setLocalWorkSize(clw::Grid(16, 16));
        kernelCalcOrientationAndAnisotropy.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelCalcOrientationAndAnisotropy.setArg(0, _secondMomentMatrix2);
        kernelCalcOrientationAndAnisotropy.setArg(1, _oriAni);
        _gpuComputeModule->queue().asyncRunKernel(kernelCalcOrientationAndAnisotropy);

        clw::Kernel kernelKuwahara = _gpuComputeModule->acquireKernel(
            format.order == clw::EChannelOrder::R ? _kidAnisotropicKuwahara : _kidAnisotropicKuwaharaRgb);
        kernelKuwahara.setLocalWorkSize(clw::Grid(16, 16));
        kernelKuwahara.setRoundedGlobalWorkSize(clw::Grid(width, height));
        kernelKuwahara.setArg(0, deviceSrc);
        kernelKuwahara.setArg(1, _kernelImage);
        kernelKuwahara.setArg(2, _oriAni);
        kernelKuwahara.setArg(3, deviceDest);
        kernelKuwahara.setArg(4, _radius);
        _gpuComputeModule->queue().runKernel(kernelKuwahara);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::DeviceImage, "source", "Source", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::DeviceImage, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Integer, "Radius", "min: 1, max:20" },
            { EPropertyType::Integer, "N", "min: 3, max:8" },
            { EPropertyType::Double, "Smoothing", "min:0.00, max:1.00" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Anisotropic Kuwahara filter";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
        nodeConfig.module = "opencl";
    }

private:
    void ensureSizeIsEnough(clw::Image2D& image, int width,
        int height, const clw::ImageFormat& format)
    {
        if(image.isNull() || 
            image.width() != width || 
            image.height() != height ||
            image.format() != format)
        {
            image = _gpuComputeModule->context().createImage2D(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
                format, width, height);
        }
    }

private:
    enum class pid
    {
        Radius,
        N,
        Smoothing
    };

    int _radius;
    int _N;
    float _smoothing;

    KernelID _kidAnisotropicKuwahara;
    KernelID _kidAnisotropicKuwaharaRgb;
    KernelID _kidCalcStructureTensor;
    KernelID _kidCalcStructureTensorRgb;
    KernelID _kidConvolutionGaussian;
    KernelID _kidCalcOrientationAndAnisotropy;

    clw::Image2D _kernelImage;
    clw::Image2D _secondMomentMatrix;
    clw::Image2D _secondMomentMatrix2;
    clw::Image2D _oriAni;

    bool _recreateKernelImage;
};

#endif

void registerGpuKuwaharaFilter(NodeSystem* nodeSystem)
{
#if defined(HAVE_OPENCL)
    typedef DefaultNodeFactory<GpuKuwaharaFilterNodeType> GpuKuwaharaFilterFactory;
    typedef DefaultNodeFactory<GpuGeneralizedKuwaharaFilterNodeType> GpuGeneralizedKuwaharaFilterFactory;
    typedef DefaultNodeFactory<GpuAnisotropicKuwaharaFilterNodeType> GpuAnisotropicKuwaharaFilterFactory;

    nodeSystem->registerNodeType("OpenCL/Filters/Kuwahara filter", 
        std::unique_ptr<NodeFactory>(new GpuKuwaharaFilterFactory()));
    nodeSystem->registerNodeType("OpenCL/Filters/Generalized Kuwahara filter", 
        std::unique_ptr<NodeFactory>(new GpuGeneralizedKuwaharaFilterFactory()));
    nodeSystem->registerNodeType("OpenCL/Filters/Anisotropic Kuwahara filter", 
        std::unique_ptr<NodeFactory>(new GpuAnisotropicKuwaharaFilterFactory()));
#endif
}
