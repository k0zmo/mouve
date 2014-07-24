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
#include "Kommon/StringUtils.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class CannyEdgeDetectorNodeType : public NodeType
{
public:
    CannyEdgeDetectorNodeType()
        : _threshold(10)
        , _ratio(3)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::ImageMono);
        addProperty("Threshold", _threshold)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 100.0))
            .setUiHints("min:0.0, max:100.0, decimals:3");
        addProperty("Ratio", _ratio)
            .setValidator(make_validator<MinPropertyValidator<double>>(0.0))
            .setUiHints("min:0.0, decimals:3");
        setDescription("Detects edges in input image using Canny detector");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const cv::Mat& input = reader.readSocket(0).getImage();
        cv::Mat& output = writer.acquireSocket(0).getImageMono();

        if(input.rows == 0 || input.cols == 0)
            return ExecutionStatus(EStatus::Ok);;

        cv::Canny(input, output, _threshold, _threshold*_ratio, 3);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<double> _threshold;
    TypedNodeProperty<double> _ratio;
};

class HoughLinesNodeType : public NodeType
{
public:
    HoughLinesNodeType()
        : _threshold(100)
        , _rhoResolution(1.0f)
        , _thetaResolution(1.0f)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Lines", ENodeFlowDataType::Array);
        addProperty("Accumulator threshold", _threshold);
        addProperty("Rho resolution", _rhoResolution)
            .setValidator(make_validator<GreaterPropertyValidator<float>>(0.0f))
            .setUiHints("min:0.01");
        addProperty("Theta resolution", _thetaResolution)
            .setValidator(make_validator<GreaterPropertyValidator<float>>(0.0f))
            .setUiHints("min:0.01");
        setDescription("Finds lines in a binary image using the standard Hough transform.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        cv::Mat& lines = writer.acquireSocket(0).getArray();

        // Validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        std::vector<cv::Vec2f> linesVector;
        cv::HoughLines(src, linesVector, _rhoResolution, CV_PI/180 * _thetaResolution, _threshold);
        lines.create(static_cast<int>(linesVector.size()), 2, CV_32F);

        float* linesPtr = lines.ptr<float>();
        for(const auto& line : linesVector)
        {
            linesPtr[0] = line[0];
            linesPtr[1] = line[1];
            linesPtr += 2;
        }

        return ExecutionStatus(EStatus::Ok,
            string_format("Lines detected: %d", (int) linesVector.size()));
    }

private:
    TypedNodeProperty<int> _threshold;
    TypedNodeProperty<float> _rhoResolution;
    TypedNodeProperty<float> _thetaResolution;
};

class HoughCirclesNodeType : public NodeType
{
public:
    HoughCirclesNodeType()
        : _dp(2.0)
        , _cannyThreshold(200.0)
        , _accThreshold(100.0)
        , _minRadius(0)
        , _maxRadius(0)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Circles", ENodeFlowDataType::Array);
        addProperty("Accumulator resolution", _dp)
            .setValidator(make_validator<GreaterPropertyValidator<double>>(0.0))
            .setUiHints("min:0.01");
        addProperty("Canny threshold", _cannyThreshold)
            .setValidator(make_validator<GreaterPropertyValidator<double>>(0.0))
            .setUiHints("min:0.0");
        addProperty("Centers threshold", _accThreshold)
            .setValidator(make_validator<GreaterPropertyValidator<double>>(0.0))
            .setUiHints("min:0.01");
        addProperty("Min. radius", _minRadius)
            .setValidator(make_validator<MinPropertyValidator<int>>(0))
            .setUiHints("min:0");
        addProperty("Max. radius", _maxRadius)
            .setValidator(make_validator<MinPropertyValidator<int>>(0))
            .setUiHints("min:0");
        setDescription("Finds circles in a grayscale image using the Hough transform.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        cv::Mat& circles = writer.acquireSocket(0).getArray();

        // Validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::HoughCircles(src, circles, CV_HOUGH_GRADIENT, _dp, 
            src.rows/8, _cannyThreshold, _accThreshold, _minRadius, _maxRadius);

        return ExecutionStatus(EStatus::Ok, 
            string_format("Circles detected: %d", circles.cols));
    }

private:
    TypedNodeProperty<double> _dp;
    TypedNodeProperty<double> _cannyThreshold;
    TypedNodeProperty<double> _accThreshold;
    TypedNodeProperty<int> _minRadius;
    TypedNodeProperty<int> _maxRadius;
};

REGISTER_NODE("Features/Hough Circles", HoughCirclesNodeType)
REGISTER_NODE("Features/Hough Lines", HoughLinesNodeType)
REGISTER_NODE("Features/Canny edge detector", CannyEdgeDetectorNodeType)
