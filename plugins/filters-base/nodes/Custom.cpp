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

#include "impl/Predefined.h"

#include <opencv2/imgproc/imgproc.hpp>

class PredefinedConvolutionNodeType : public NodeType
{
public:
    PredefinedConvolutionNodeType() : _filterType(EPredefinedConvolutionType::NoOperation)
    {
        addInput("Source", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Kernel", _filterType)
            .setUiHints("item: No operation, item: Average, item: Gaussian, item: Mean removal,"
                        "item: Robert's cross 45, item: Robert's cross 135, item: Laplacian,"
                        "item: Prewitt horizontal, item: Prewitt vertical, "
                        "item: Sobel horizontal, item: Sobel vertical, "
                        "item: Scharr horizontal, item: Scharr vertical");
        setDescription("Performs image convolution using one of predefined convolution kernel.");
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
        cv::Mat kernel = predefinedConvolutionKernel(
            _filterType.cast<Enum>().cast<EPredefinedConvolutionType>());
        bool weight0 = cv::sum(kernel) == cv::Scalar(0);
        int ddepth = weight0 ? CV_16S : -1;

        cv::filter2D(input, output, ddepth, kernel);

        if (weight0)
            cv::convertScaleAbs(output, output);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<EPredefinedConvolutionType> _filterType;
};

class CustomConvolutionNodeType : public NodeType
{
public:
    CustomConvolutionNodeType() : _coeffs(1), _scaleAbs(true)
    {
        addInput("Source", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Coefficients", _coeffs);
        addProperty("Scale absolute", _scaleAbs);
        setDescription("Performs image convolution using user-given convolution kernel.");
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
        int ddepth = _scaleAbs ? CV_16S : -1;

        cv::Mat kernel(3, 3, CV_64FC1, _coeffs.cast<Matrix3x3>().v);
        cv::filter2D(input, output, ddepth, kernel);

        if (_scaleAbs)
            cv::convertScaleAbs(output, output);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<Matrix3x3> _coeffs;
    TypedNodeProperty<bool> _scaleAbs;
};

REGISTER_NODE("Filters/Convolution", CustomConvolutionNodeType)
REGISTER_NODE("Filters/Pre-defined convolution", PredefinedConvolutionNodeType)
