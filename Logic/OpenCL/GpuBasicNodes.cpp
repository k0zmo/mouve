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

class GpuUploadImageNodeType : public GpuNodeType
{
public:
    GpuUploadImageNodeType()
        : _usePinnedMemory(false),
          _pinnedMemory(nullptr)
    {
        addInput("Host image", ENodeFlowDataType::Image);
        addOutput("Device image", ENodeFlowDataType::DeviceImage);
        addProperty("Use pinned memory", _usePinnedMemory);
        setDescription("Uploads given image from host to device (GPU) memory");
        setModule("opencl");
    }

    ~GpuUploadImageNodeType()
    {
        if (_pinnedMemory && !_pinnedBuffer.isNull())
        {
            _gpuComputeModule->queue().asyncUnmap(_pinnedBuffer, _pinnedMemory);
        }
    }

    bool postInit() override
    {
        _kidConvertBufferRgbToImageRgba = _gpuComputeModule->registerKernel("convertBufferRgbToImageRgba", "color.cl");
        return _kidConvertBufferRgbToImageRgba != InvalidKernelID;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const cv::Mat& hostImage = reader.readSocket(0).getImage();
        clw::Image2D& deviceImage = writer.acquireSocket(0).getDeviceImage();

        if(hostImage.channels() != 1 && hostImage.channels() != 3)
            return ExecutionStatus(EStatus::Error, "Invalid number of channels (should be 1 or 3)");

        auto devChannels = [](clw::Image2D& image) {
            // Make 4 bytes per elem 3-channel image, otherwise its ok
            return image.bytesPerElement() == 4 ? 3 : image.bytesPerElement();
        };

        if(deviceImage.isNull()
            || deviceImage.width() != hostImage.cols
            || deviceImage.height() != hostImage.rows
            || devChannels(deviceImage) != hostImage.channels())
        {
            clw::EChannelOrder channelOrder = hostImage.channels() == 1
                ? clw::EChannelOrder::R
                : clw::EChannelOrder::RGBA;
            deviceImage = _gpuComputeModule->context().createImage2D(
                clw::EAccess::ReadWrite, clw::EMemoryLocation::Device,
                clw::ImageFormat(channelOrder, clw::EChannelType::Normalized_UInt8),
                hostImage.cols, hostImage.rows);
        }

        if(_usePinnedMemory)
        {
            size_t pinnedBufferSize = hostImage.cols * hostImage.rows * hostImage.channels() * sizeof(uchar);

            if(_pinnedBuffer.isNull() || pinnedBufferSize != _pinnedBuffer.size())
            {
                _pinnedBuffer = _gpuComputeModule->context().createBuffer(clw::EAccess::ReadOnly,
                    clw::EMemoryLocation::AllocHostMemory, pinnedBufferSize);
                _pinnedMemory = nullptr;
            }
            if(!_pinnedMemory)
            {
                _pinnedMemory = _gpuComputeModule->queue().mapBuffer(_pinnedBuffer, clw::EMapAccess::Write);
                if(!_pinnedMemory)
                    return ExecutionStatus(EStatus::Error, "Couldn't mapped pinned memory for device image transfer");
            }

            copyToPinnedMemory(hostImage);
        }

        if(hostImage.channels() == 1)
        {
            // Simple copy will suffice
            bool result = _gpuComputeModule->queue().writeImage2D(deviceImage, _usePinnedMemory ? _pinnedMemory : hostImage.data,
                clw::Rect(0, 0, hostImage.cols, hostImage.rows), static_cast<int>(hostImage.step));
            return ExecutionStatus(result ? EStatus::Ok : EStatus::Error);
        }
        else
        {
            // Copy to intermediate buffer (BGR) and run kernel to convert it to RGBA image
            clw::Kernel kernelConvertBufferRgbToImageRgba = _gpuComputeModule->acquireKernel(_kidConvertBufferRgbToImageRgba);
            int intermediateBufferPitch = hostImage.cols * hostImage.channels() * sizeof(uchar);
            int intermediateBufferSize = intermediateBufferPitch * hostImage.rows;

            if(_intermediateBuffer.isNull() || _intermediateBuffer.size() != intermediateBufferSize)
            {
                _intermediateBuffer = _gpuComputeModule->context().createBuffer(clw::EAccess::ReadOnly,
                    clw::EMemoryLocation::Device, intermediateBufferSize);
            }

            _gpuComputeModule->queue().asyncWriteBuffer(_intermediateBuffer, _usePinnedMemory ? _pinnedMemory : hostImage.data);

            kernelConvertBufferRgbToImageRgba.setLocalWorkSize(16, 16);
            kernelConvertBufferRgbToImageRgba.setRoundedGlobalWorkSize(hostImage.cols, hostImage.rows);
            kernelConvertBufferRgbToImageRgba.setArg(0, _intermediateBuffer);
            kernelConvertBufferRgbToImageRgba.setArg(1, intermediateBufferPitch);
            kernelConvertBufferRgbToImageRgba.setArg(2, deviceImage);

            _gpuComputeModule->queue().runKernel(kernelConvertBufferRgbToImageRgba);

            return ExecutionStatus(EStatus::Ok);
        }
    }

private:
    void copyToPinnedMemory(const cv::Mat& hostImage)
    {
        if((int) hostImage.step != hostImage.cols)
        {
            uchar* dst = (uchar*)_pinnedMemory;
            const uchar* src = (uchar*)hostImage.data;
            const size_t stride = hostImage.step;
            for(int row = 0; row < hostImage.rows; ++row)
            {
                memcpy(dst, src, stride);
                dst += stride;
                src += stride;
            }
        }
        else
        {
            memcpy(_pinnedMemory, hostImage.data, hostImage.step * hostImage.rows);
        }
    }

private:
    clw::Buffer _pinnedBuffer;
    void* _pinnedMemory;
    clw::Buffer _intermediateBuffer;
    KernelID _kidConvertBufferRgbToImageRgba;
    TypedNodeProperty<bool> _usePinnedMemory;
};

class GpuDownloadImageNodeType : public GpuNodeType
{
public:
    GpuDownloadImageNodeType()
        : _usePinnedMemory(false),
          _pinnedMemory(nullptr)
    {
        addInput("Device image", ENodeFlowDataType::DeviceImage);
        addOutput("Host image", ENodeFlowDataType::Image);
        addProperty("Use pinned memory", _usePinnedMemory);
        setDescription("Download given image device (GPU) to host memory");
        setModule("opencl");
    }

    ~GpuDownloadImageNodeType()
    {
        if (_pinnedMemory && !_pinnedBuffer.isNull())
        {
            _gpuComputeModule->queue().asyncUnmap(_pinnedBuffer, _pinnedMemory);
        }
    }

    bool postInit() override
    {
        _kidConvertImageRgbaToBufferRgb = _gpuComputeModule->registerKernel("convertImageRgbaToBufferRgb", "color.cl");
        return _kidConvertImageRgbaToBufferRgb != InvalidKernelID;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const clw::Image2D& deviceImage = reader.readSocket(0).getDeviceImage();
        cv::Mat& hostImage = writer.acquireSocket(0).getImage();

        if(deviceImage.isNull() || deviceImage.size() == 0)
            return ExecutionStatus(EStatus::Ok);

        clw::ImageFormat imageFormat = deviceImage.format();

        if(imageFormat.order != clw::EChannelOrder::R && imageFormat.order != clw::EChannelOrder::RGBA)
            return ExecutionStatus(EStatus::Error, "Wrong data type (should be one or four channel device image)");

        int width = deviceImage.width();
        int height = deviceImage.height();
        int channels = imageFormat.order == clw::EChannelOrder::R ? 1 : 3;

        hostImage.create(height, width, CV_MAKETYPE(CV_8U, channels));

        if(_usePinnedMemory)
        {
            size_t pinnedBufferSize = hostImage.cols * hostImage.rows * hostImage.channels() * sizeof(uchar);

            if(_pinnedBuffer.isNull() || pinnedBufferSize != _pinnedBuffer.size())
            {
                _pinnedBuffer = _gpuComputeModule->context().createBuffer(clw::EAccess::ReadOnly,
                    clw::EMemoryLocation::AllocHostMemory, pinnedBufferSize);
                _pinnedMemory = nullptr;
            }
            if(!_pinnedMemory)
            {
                _pinnedMemory = _gpuComputeModule->queue().mapBuffer(_pinnedBuffer, clw::EMapAccess::Read);
                if(!_pinnedMemory)
                    return ExecutionStatus(EStatus::Error, "Couldn't mapped pinned memory for device image transfer");
            }
        }

        if(channels == 1)
        {
            // Simple copy will suffice
            bool res = _gpuComputeModule->queue().readImage2D(deviceImage, _usePinnedMemory ? _pinnedMemory : hostImage.data,
                clw::Rect(0, 0, hostImage.cols, hostImage.rows), static_cast<int>(hostImage.step));
            if (_usePinnedMemory)
                copyFromPinnedMemory(hostImage);
            return ExecutionStatus(res ? EStatus::Ok : EStatus::Error);
        }
        else
        {
            // Convert image RGBA to intermediate buffer (BGR) and copy it to host image
            clw::Kernel kernelConvertImageRgbaToBufferRgb = _gpuComputeModule->acquireKernel(_kidConvertImageRgbaToBufferRgb);
            int intermediateBufferPitch = hostImage.cols * hostImage.channels() * sizeof(uchar);
            int intermediateBufferSize = intermediateBufferPitch * hostImage.rows;

            if(_intermediateBuffer.isNull() || _intermediateBuffer.size() != intermediateBufferSize)
            {
                _intermediateBuffer = _gpuComputeModule->context().createBuffer(clw::EAccess::WriteOnly,
                    clw::EMemoryLocation::Device, intermediateBufferSize);
            }

            kernelConvertImageRgbaToBufferRgb.setLocalWorkSize(16, 16);
            kernelConvertImageRgbaToBufferRgb.setRoundedGlobalWorkSize(hostImage.cols, hostImage.rows);
            kernelConvertImageRgbaToBufferRgb.setArg(0, deviceImage);
            kernelConvertImageRgbaToBufferRgb.setArg(1, _intermediateBuffer);
            kernelConvertImageRgbaToBufferRgb.setArg(2, intermediateBufferPitch);

            _gpuComputeModule->queue().asyncRunKernel(kernelConvertImageRgbaToBufferRgb);
            _gpuComputeModule->queue().readBuffer(_intermediateBuffer, _usePinnedMemory ? _pinnedMemory : hostImage.data);
            if (_usePinnedMemory)
                copyFromPinnedMemory(hostImage);
            return ExecutionStatus(EStatus::Ok);
        }
    }

private:
    void copyFromPinnedMemory(cv::Mat& hostImage)
    {
        if((int) hostImage.step != hostImage.cols)
        {
            uchar* dst = (uchar*)hostImage.data;
            const uchar* src = (uchar*)_pinnedMemory;
            const size_t stride = hostImage.step;
            for(int row = 0; row < hostImage.rows; ++row)
            {
                memcpy(dst, src, stride);
                dst += stride;
                src += stride;
            }
        }
        else
        {
            memcpy(hostImage.data, _pinnedMemory, hostImage.step * hostImage.rows);
        }
    }

private:
    clw::Buffer _pinnedBuffer;
    void* _pinnedMemory;
    clw::Buffer _intermediateBuffer;
    KernelID _kidConvertImageRgbaToBufferRgb;
    TypedNodeProperty<bool> _usePinnedMemory;
};

class GpuUploadArrayNodeType : public GpuNodeType
{
public:
    GpuUploadArrayNodeType()
    {
        addInput("Host array", ENodeFlowDataType::Array);
        addOutput("Device array", ENodeFlowDataType::DeviceArray);
        setDescription("Uploads given array from host to device (GPU) memory");
        setModule("opencl");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const cv::Mat& hostArray = reader.readSocket(0).getArray();
        DeviceArray& deviceArray = writer.acquireSocket(0).getDeviceArray();

        if(hostArray.empty())
            return ExecutionStatus(EStatus::Ok);

        deviceArray = DeviceArray::create(_gpuComputeModule->context(), clw::EAccess::ReadWrite,
            clw::EMemoryLocation::Device, hostArray.cols, hostArray.rows,
            DeviceArray::matToDeviceType(hostArray.type()));
        clw::Event evt = deviceArray.upload(_gpuComputeModule->queue(), hostArray.data);
        evt.waitForFinished();

        return ExecutionStatus(EStatus::Ok);
    }
};

class GpuDownloadArrayNodeType : public GpuNodeType
{
public:
    GpuDownloadArrayNodeType()
    {
        addInput("Device array", ENodeFlowDataType::DeviceArray);
        addOutput("Host array", ENodeFlowDataType::Array);
        setDescription("Download given array device (GPU) to host memory");
        setModule("opencl");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const DeviceArray& deviceArray = reader.readSocket(0).getDeviceArray();
        cv::Mat& hostArray = writer.acquireSocket(0).getArray();

        if(deviceArray.isNull() || deviceArray.size() == 0)
            return ExecutionStatus(EStatus::Ok);

        clw::Event evt = deviceArray.download(_gpuComputeModule->queue(), hostArray);
        evt.waitForFinished();

        return ExecutionStatus(EStatus::Ok);
    }
};

REGISTER_NODE("OpenCL/Download array", GpuDownloadArrayNodeType)
REGISTER_NODE("OpenCL/Upload array", GpuUploadArrayNodeType)
REGISTER_NODE("OpenCL/Download image", GpuDownloadImageNodeType)
REGISTER_NODE("OpenCL/Upload image", GpuUploadImageNodeType)

#endif
