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

#include <opencv2/imgproc/imgproc.hpp>

#include "CV.h"

class BoxFilterNodeType : public NodeType
{
public:
    BoxFilterNodeType()
        : _kernelSize(3)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Blurring kernel size", _kernelSize)
            .setValidator(make_validator<MinPropertyValidator<int>>(2))
            .setUiHints("min:2, step:1");
        setDescription("Blurs an image using box filter.");
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
        cv::boxFilter(input, output, CV_8U, 
            cv::Size(_kernelSize, _kernelSize),
            cv::Point(-1,1), true, cv::BORDER_REFLECT_101);
        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<int> _kernelSize;
};

class BilateralFilterNodeType : public NodeType
{
public:
    BilateralFilterNodeType()
        : _diameter(9)
        , _sigmaColor(50)
        , _sigmaSpace(50)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Diameter of pixel neighborhood", _diameter);
        addProperty("Filter sigma in color space", _sigmaColor);
        addProperty("Filter sigma in the coordinate space", _sigmaSpace);
        setDescription("Applies the bilateral filter to an image.");
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
        cv::bilateralFilter(input, output, _diameter,
            _sigmaColor, _sigmaSpace, cv::BORDER_REFLECT_101);
        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<int> _diameter;
    TypedNodeProperty<double> _sigmaColor;
    TypedNodeProperty<double> _sigmaSpace;
};

class GaussianFilterNodeType : public NodeType
{
public:
    GaussianFilterNodeType()
        : _sigma(1.2)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Sigma", _sigma)
            .setValidator(make_validator<GreaterPropertyValidator<double>>(0.0))
            .setUiHints("min:0.1, step:0.1");
        setDescription("Blurs an image using a Gaussian filter.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const cv::Mat& input = reader.readSocket(0).getImage();
        cv::Mat& output = writer.acquireSocket(0).getImage();

        if(!input.data)
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        // sigma = 0.3 * ((ksize-1)*0.5 - 1) + 0.8
        // int ksize = cvRound((20*_sigma - 7)/3);
        //cv::Mat kernel = cv::getGaussianKernel(ksize, _sigma, CV_64F);
        cv::GaussianBlur(input, output, cv::Size(0,0), _sigma, 0);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<double> _sigma;
};

class MedianFilterNodeType : public NodeType
{
public:
    MedianFilterNodeType()
        : _apertureSize(3)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Aperture linear size", _apertureSize)
            .setValidator(make_validator<FuncValidator>([](const NodeProperty& np) {
                // Let aperture size be odd
                auto v = np.cast<int>();
                return (v >= 3) && ((v % 2) != 0);
            }))
            .setUiHints("min:3, step:2");
        setDescription("Blurs an image using the median filter.");
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
        cv::medianBlur(input, output, _apertureSize);
        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<int> _apertureSize;
};

class LaplacianFilterNodeType : public NodeType
{
public:
    LaplacianFilterNodeType()
        : _apertureSize(1)
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
        if(input.empty())
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
    SobelFilterNodeType()
        : _apertureSize(3)
        , _order(1)
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
        if(input.empty())
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
        if(input.empty())
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

class PredefinedConvolutionNodeType : public NodeType
{
public:
    PredefinedConvolutionNodeType()
        : _filterType(cvu::EPredefinedConvolutionType::NoOperation)
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
        if(input.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::Mat kernel = cvu::predefinedConvolutionKernel(
            _filterType.cast<Enum>().cast<cvu::EPredefinedConvolutionType>());
        bool weight0 = cv::sum(kernel) == cv::Scalar(0);
        int ddepth = weight0 ? CV_16S : -1;

        cv::filter2D(input, output, ddepth, kernel);

        if(weight0)
            cv::convertScaleAbs(output, output);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<cvu::EPredefinedConvolutionType> _filterType;
};

class CustomConvolutionNodeType : public NodeType
{
public:
    CustomConvolutionNodeType()
        : _coeffs(1)
        , _scaleAbs(true)
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
        if(input.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        int ddepth = _scaleAbs ? CV_16S : -1;

        cv::Mat kernel(3, 3, CV_64FC1, _coeffs.cast<Matrix3x3>().v);
        cv::filter2D(input, output, ddepth, kernel);

        if(_scaleAbs)
            cv::convertScaleAbs(output, output);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<Matrix3x3> _coeffs;
    TypedNodeProperty<bool> _scaleAbs;
};

REGISTER_NODE("Filters/Convolution", CustomConvolutionNodeType)
REGISTER_NODE("Filters/Pre-defined convolution", PredefinedConvolutionNodeType)
REGISTER_NODE("Filters/Scharr filter", ScharrFilterNodeType)
REGISTER_NODE("Filters/Sobel filter", SobelFilterNodeType)
REGISTER_NODE("Filters/Laplacian filter", LaplacianFilterNodeType)
REGISTER_NODE("Filters/Median filter", MedianFilterNodeType)
REGISTER_NODE("Filters/Gaussian filter", GaussianFilterNodeType)
REGISTER_NODE("Filters/Bilateral filter", BilateralFilterNodeType)
REGISTER_NODE("Filters/Box filter", BoxFilterNodeType)
