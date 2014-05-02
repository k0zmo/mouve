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

class BinarizationNodeType : public NodeType
{
public:
    BinarizationNodeType()
        : _threshold(128)
        , _inv(false)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Threshold:
            _threshold = newValue.toInt();
            return true;
        case pid::Invert:
            _inv = newValue.toBool();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Threshold: return _threshold;
        case pid::Invert: return _inv;
        }

        return NodeProperty();
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImageMono();

        // Validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);
        if(_threshold < 0 || _threshold > 255)
            return ExecutionStatus(EStatus::Error, "Bad threshold value");

        // Do stuff
        int type = _inv ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
        cv::threshold(src, dst, (double) _threshold, 255, type);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::ImageMono, "source", "Source", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::ImageMono, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Integer, "Threshold", "min:0, max:255" },
            { EPropertyType::Boolean, "Inverted", "" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Applies a fixed-level threshold to each pixel element.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
    }

private:
    enum class pid
    {
        Threshold,
        Invert
    };

    int _threshold;
    bool _inv;
};

class OtsuThresholdingNodeType : public NodeType
{
public:
    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImageMono();

        // Validate inputs
        if(src.empty() || src.type() != CV_8U)
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::threshold(src, dst, 0, 255, cv::THRESH_OTSU);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::ImageMono, "source", "Source", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::ImageMono, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        nodeConfig.description = "Applies optimal threshold value using Otsu's algorithm to each pixel element.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
    }
};

REGISTER_NODE("Segmentation/Otsu's thresholding", OtsuThresholdingNodeType)
REGISTER_NODE("Segmentation/Binarization", BinarizationNodeType)
