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

#include "Logic/NodeType.h"
#include "Logic/NodeFactory.h"

#include <opencv2/imgproc/imgproc.hpp>

class LaplacianFilterNodeType : public NodeType
{
public:
    LaplacianFilterNodeType() : _apertureSize(1)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Aperture linear size", _apertureSize)
            .setValidator(make_validator<FuncValidator>([](const NodeProperty& np) {
                // Let aperture size be odd
                auto v = np.cast<int>();
                return (v >= 1) && (v <= 31) && ((v % 2) != 0);
            }))
            .setUiHints("min:1, max: 31, step:2");
        setDescription("Calculates the Laplacian of an image.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& input = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImage();

        // Validate inputs
        if (input.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::Laplacian(input, output, CV_8U, _apertureSize);
        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<int> _apertureSize;
};

class SobelFilterNodeType : public NodeType
{
public:
    SobelFilterNodeType() : _apertureSize(3), _order(1)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Aperture size", _apertureSize)
            .setValidator(make_validator<FuncValidator>([](const NodeProperty& np) {
                // Let aperture size be odd
                auto v = np.cast<int>();
                return (v >= 1) && (v <= 7) && ((v % 2) != 0);
            }))
            .setUiHints("min:1, max: 7, step:2");
        addProperty("Derivatives order", _order)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 3))
            .setUiHints("min:1, max: 3, step:1");
        setDescription("Calculates the first image derivatives using Sobel operator.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& input = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImage();

        // Validate inputs
        if (input.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::Mat xgrad;
        cv::Sobel(input, xgrad, CV_16S, _order, 0, _apertureSize);
        cv::convertScaleAbs(xgrad, xgrad);

        cv::Mat ygrad;
        cv::Sobel(input, ygrad, CV_16S, 0, _order, _apertureSize);
        cv::convertScaleAbs(ygrad, ygrad);

        cv::addWeighted(xgrad, 0.5, ygrad, 0.5, 0, output);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<int> _apertureSize;
    TypedNodeProperty<int> _order;
};

class ScharrFilterNodeType : public NodeType
{
public:
    ScharrFilterNodeType()
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        setDescription("Calculates the first image derivatives using Scharr operator.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& input = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImage();

        // Validate inputs
        if (input.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::Mat xgrad;
        cv::Scharr(input, xgrad, CV_16S, 1, 0);
        cv::convertScaleAbs(xgrad, xgrad);

        cv::Mat ygrad;
        cv::Scharr(input, ygrad, CV_16S, 0, 1);
        cv::convertScaleAbs(ygrad, ygrad);

        cv::addWeighted(xgrad, 0.5, ygrad, 0.5, 0, output);

        return ExecutionStatus(EStatus::Ok);
    }
};

REGISTER_NODE("Filters/Scharr filter", ScharrFilterNodeType)
REGISTER_NODE("Filters/Sobel filter", SobelFilterNodeType)
REGISTER_NODE("Filters/Laplacian filter", LaplacianFilterNodeType)
