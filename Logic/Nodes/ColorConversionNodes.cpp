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
#include "Kommon/Utils.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "CV.h"

class GrayToRgbNodeType : public NodeType
{
public:
    GrayToRgbNodeType()
    {
        addInput("Gray", ENodeFlowDataType::Image);
        addOutput("Color", ENodeFlowDataType::ImageRgb);
        setDescription("Converts gray 1-channel image to 3-channel image");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& input = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImageRgb();

        // Validate inputs
        if(input.empty())
            return ExecutionStatus(EStatus::Ok);

        if(input.channels() == 3)
        {
            // no-op
            output = input;
        }
        else
        {
            if(input.data == output.data)
                output = cv::Mat();
            // Do stuff
            cv::cvtColor(input, output, CV_GRAY2BGR);
        }

        return ExecutionStatus(EStatus::Ok);
    }
};

class RgbToGrayNodeType : public NodeType
{
public:
    RgbToGrayNodeType()
    {
        addInput("Color", ENodeFlowDataType::Image);
        addOutput("Gray", ENodeFlowDataType::ImageMono);
        setDescription("Converts color 3-channel image to 1-channel gray image");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& input = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImageMono();

        // Validate inputs
        if(input.empty())
            return ExecutionStatus(EStatus::Ok);

        if(input.channels() == 1)
        {
            //no-op
            output = input;
        }
        else
        {
            if(input.data == output.data)
                output = cv::Mat();
            // Do stuff
            cv::cvtColor(input, output, CV_BGR2GRAY);
        }

        return ExecutionStatus(EStatus::Ok);
    }
};

class BayerToGrayNodeType : public NodeType
{
public:
    BayerToGrayNodeType()
        : _BayerCode(cvu::EBayerCode::RG)
    {
        addInput("Input", ENodeFlowDataType::ImageMono);
        addOutput("Output", ENodeFlowDataType::ImageMono);
        addProperty("Bayer format", _BayerCode)
            .setUiHints("item: BG, item: GB, item: RG, item: GR");
        setDescription("Performs demosaicing from Bayer pattern image to gray-scale image");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& input = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImageMono();

        if(input.empty())
            return ExecutionStatus(EStatus::Ok);
        
        // Do stuff
        cv::cvtColor(input, output, cvu::bayerCodeGray(_BayerCode.toEnum().cast<cvu::EBayerCode>()));

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<cvu::EBayerCode> _BayerCode;
};

class BayerToRgbNodeType : public NodeType
{
public:
    BayerToRgbNodeType()
        : _redGain(1.0)
        , _greenGain(1.0)
        , _blueGain(1.0)
        , _BayerCode(cvu::EBayerCode::RG)
    {
        addInput("Input", ENodeFlowDataType::ImageMono);
        addOutput("Output", ENodeFlowDataType::ImageRgb);
        addProperty("Bayer format", _BayerCode)
            .setUiHints("item: BG, item: GB, item: RG, item: GR");
        addProperty("Red gain", _redGain)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 4.0))
            .setUiHints("min:0.0, max:4.0, step:0.001");
        addProperty("Green gain", _greenGain)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 4.0))
            .setUiHints("min:0.0, max:4.0, step:0.001");
        addProperty("Blue gain", _blueGain)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 4.0))
            .setUiHints("min:0.0, max:4.0, step:0.001");
        setDescription("Performs demosaicing from Bayer pattern image to RGB image.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& input = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImageRgb();

        // Validate inputs
        if(input.empty())
            return ExecutionStatus(EStatus::Ok);
        
        // Do stuff
        cv::cvtColor(input, output, cvu::bayerCodeRgb(_BayerCode.toEnum().cast<cvu::EBayerCode>()));

        if(!fcmp(_redGain.toDouble(), 1.0)
        || !fcmp(_greenGain.toDouble(), 1.0) 
        || !fcmp(_blueGain.toDouble(), 1.0))
        {
            cvu::parallel_for(cv::Range(0, output.rows), [&](const cv::Range& range) 
            {
                for(int y = range.start; y < range.end; ++y)
                {
                    for(int x = 0; x < output.cols; ++x)
                    {
                        cv::Vec3b rgb = output.at<cv::Vec3b>(y, x);
                        rgb[0] = cv::saturate_cast<uchar>(rgb[0] * _blueGain);
                        rgb[1] = cv::saturate_cast<uchar>(rgb[1] * _greenGain);
                        rgb[2] = cv::saturate_cast<uchar>(rgb[2] * _redGain);
                        output.at<cv::Vec3b>(y, x) = rgb;
                    }
                }
            });
        }

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<double> _redGain;
    TypedNodeProperty<double> _greenGain;
    TypedNodeProperty<double> _blueGain;
    TypedNodeProperty<cvu::EBayerCode> _BayerCode;
};

class ContrastAndBrightnessNodeType : public NodeType
{
public:
    ContrastAndBrightnessNodeType()
        : _gain(1.0)
        , _bias(0)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Gain", _gain)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 16.0))
            .setUiHints("min:0.0, max:16.0");
        addProperty("Bias", _bias)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(-255, 255))
            .setUiHints("min:-255, max:255");
        setDescription("Adjusts contrast and brightness of input image.");
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
        if(!fcmp(_gain.toDouble(), 1.0) || _bias != 0)
        {
            output = cv::Mat(input.size(), input.type());

            if(input.channels() == 1)
            {
                cvu::parallel_for(cv::Range(0, output.rows), [&](const cv::Range& range)
                {
                    for(int y = range.start; y < range.end; ++y)
                    {
                        for(int x = 0; x < output.cols; ++x)
                        {
                            output.at<uchar>(y, x) = cv::saturate_cast<uchar>(
                                _gain * input.at<uchar>(y, x) + _bias);
                        }
                    }
                });
            }
            else if(input.channels() == 3)
            {
                cvu::parallel_for(cv::Range(0, output.rows), [&](const cv::Range& range)
                {
                    for(int y = range.start; y < range.end; ++y)
                    {
                        for(int x = 0; x < output.cols; ++x)
                        {
                            cv::Vec3b rgb = input.at<cv::Vec3b>(y, x);
                            rgb[0] = cv::saturate_cast<uchar>(_gain * rgb[0] + _bias);
                            rgb[1] = cv::saturate_cast<uchar>(_gain * rgb[1] + _bias);
                            rgb[2] = cv::saturate_cast<uchar>(_gain * rgb[2] + _bias);
                            output.at<cv::Vec3b>(y, x) = rgb;
                        }
                    }
                });
            }
        }
        else
        {
            output = input;
        }

        return ExecutionStatus(EStatus::Ok);
    }

protected:
    TypedNodeProperty<double> _gain;
    TypedNodeProperty<int> _bias;
};

REGISTER_NODE("Format conversion/Contrast & brightness", ContrastAndBrightnessNodeType)
REGISTER_NODE("Format conversion/Gray de-bayer", BayerToGrayNodeType)
REGISTER_NODE("Format conversion/RGB de-bayer", BayerToRgbNodeType)
REGISTER_NODE("Format conversion/Gray to RGB", GrayToRgbNodeType)
REGISTER_NODE("Format conversion/RGB to gray", RgbToGrayNodeType)
