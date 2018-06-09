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

#include "Logic/NodeType.h"
#include "Logic/NodeFactory.h"

#include <fmt/core.h>
#include <opencv2/imgproc/imgproc.hpp>

class RotateImageNodeType : public NodeType
{
public:
    RotateImageNodeType()
        : _angle(0)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Angle", _angle)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 360.0))
            .setUiHints("min:0, max:360, step:0.1");
        setDescription("Applies rotation transformation to a given image.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& input = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImage();

        // Validate inputs
        if(input.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::Point2f center(input.cols * 0.5f, input.rows * 0.5f);
        cv::Mat rotationMat = cv::getRotationMatrix2D(center, _angle, 1);
        cv::warpAffine(input, output, rotationMat, input.size(), cv::INTER_CUBIC);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<double> _angle;
};

class ScaleImageNodeType : public NodeType
{
public:
    ScaleImageNodeType()
        : _scale(1.0)
        , _inter(EInterpolationMethod::eimArea)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Scale", _scale)
            .setValidator(make_validator<GreaterPropertyValidator<double>>(0.0))
            .setUiHints("min:0, step:0.1");
        addProperty("Interpolation method", _inter)
            .setUiHints("item: Nearest neighbour, item: Linear, item: Cubic, item: Area, item: Lanczos");
        setDescription("Resizes given image.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& input = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImage();

        // Validate inputs
        if(input.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::Size dstSize(int(input.cols * _scale), int(input.rows * _scale));
        cv::resize(input, output, dstSize, 0, 0, _inter.cast<Enum>().data());

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Output image width: {}\nOutput image height: {}",
                                           output.cols, output.rows));
    }

private:

    enum class EInterpolationMethod
    {
        eimNearestNeighbour = cv::INTER_NEAREST,
        eimLinear           = cv::INTER_LINEAR,
        eimCubic            = cv::INTER_CUBIC,
        eimArea             = cv::INTER_AREA,
        eimLanczos          = cv::INTER_LANCZOS4
    };

    TypedNodeProperty<double> _scale;
    TypedNodeProperty<EInterpolationMethod >_inter;
};

class DownsampleNodeType : public NodeType
{
public:
    DownsampleNodeType()
    {
        addInput("Source", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        setDescription("Blurs an image and downsamples it.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImage();

        // Acquire output sockets
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::pyrDown(src, dst, cv::Size(src.cols/2, src.rows/2));

        return ExecutionStatus(EStatus::Ok, fmt::format("Image size: {}x{}\n", dst.cols, dst.rows));
    }
};

class UpsampleNodeType : public NodeType
{
public:
    UpsampleNodeType()
    {
        addInput("Source", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        setDescription("Upsamples an image and then blurs it.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImage();

        // Acquire output sockets
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::pyrUp(src, dst, cv::Size(src.cols*2, src.rows*2));

        return ExecutionStatus(EStatus::Ok, fmt::format("Image size: {}x{}\n", dst.cols, dst.rows));
    }
};

class MaskedImageNodeType : public NodeType
{
public:
    MaskedImageNodeType()
    {
        addInput("Image", ENodeFlowDataType::Image);
        addInput("Mask", ENodeFlowDataType::ImageMono);
        addOutput("Output", ENodeFlowDataType::Image);
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImage();
        const cv::Mat& mask = reader.readSocket(1).getImageMono();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImage();

        // Validate inputs
        if(src.empty() || mask.empty())
            return ExecutionStatus(EStatus::Ok);
        if(src.size() != mask.size())
            return ExecutionStatus(EStatus::Error, "Source");

        // Do stuff
        dst = cv::Mat(dst.size(), src.type(), cv::Scalar(0));
        src.copyTo(dst, mask);

        return ExecutionStatus(EStatus::Ok);
    }
};

REGISTER_NODE("Transformations/Masked image", MaskedImageNodeType)
REGISTER_NODE("Transformations/Upsample", UpsampleNodeType)
REGISTER_NODE("Transformations/Downsample", DownsampleNodeType)
REGISTER_NODE("Transformations/Rotate", RotateImageNodeType)
REGISTER_NODE("Transformations/Scale", ScaleImageNodeType)
