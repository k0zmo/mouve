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

class AddNodeType : public NodeType
{
public:
    AddNodeType()
        : _alpha(0.5)
        , _beta(0.5)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Alpha:
            _alpha = newValue.toDouble();
            return true;
        case pid::Beta:
            _beta = newValue.toDouble();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Alpha: return _alpha;
        case pid::Beta: return _beta;
        }

        return NodeProperty();
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src1 = reader.readSocket(0).getImage();
        const cv::Mat& src2 = reader.readSocket(1).getImage();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImage();

        // Validate inputs
        if(src1.empty() || src2.empty())
            return ExecutionStatus(EStatus::Ok);
        if(src1.size() != src2.size())
            return ExecutionStatus(EStatus::Error, "Inputs with different sizes");

        // Do stuff
        cv::addWeighted(src1, _alpha, src2, _beta, 0, dst);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::Image, "source1", "X", "" },
            { ENodeFlowDataType::Image, "source2", "Y", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Image, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Double, "Alpha", "min:0.0, max:1.0, decimals:3, step:0.1" },
            { EPropertyType::Double, "Beta",  "min:0.0, max:1.0, decimals:3, step:0.1" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Adds one image to another with specified weights: alpha * x + beta * y.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
    }

private:
    enum class pid
    {
        Alpha,
        Beta
    };

    double _alpha;
    double _beta;
};

class AbsoluteNodeType : public NodeType
{
public:
    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImage();

        // Validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        dst = cv::abs(src);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::Image, "source", "Source", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Image, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        nodeConfig.description = "Calculates an absolute value of each pixel element in given image: y = abs(x).";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
    }
};

class SubtractNodeType : public NodeType
{
public:
    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src1 = reader.readSocket(0).getImage();
        const cv::Mat& src2 = reader.readSocket(1).getImage();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImage();

        // Validate inputs
        if(src1.empty() || src2.empty())
            return ExecutionStatus(EStatus::Ok);
        if(src1.size() != src2.size())
            return ExecutionStatus(EStatus::Error, "Inputs with different sizes");

        // Do stuff
        cv::subtract(src1, src2, dst);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::Image, "source1", "X", "" },
            { ENodeFlowDataType::Image, "source2", "Y", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Image, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        nodeConfig.description = "Subtracts (per-element) one image from another : x - y";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
    }
};

class AbsoluteDifferenceNodeType : public NodeType
{
public:
    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src1 = reader.readSocket(0).getImage();
        const cv::Mat& src2 = reader.readSocket(1).getImage();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImage();

        // Validate inputs
        if(src1.empty() || src2.empty())
            return ExecutionStatus(EStatus::Ok);
        if(src1.size() != src2.size())
            return ExecutionStatus(EStatus::Error, "Inputs with different sizes");

        // Do stuff
        cv::absdiff(src1, src2, dst);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::Image, "source1", "X", "" },
            { ENodeFlowDataType::Image, "source2", "Y", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Image, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        nodeConfig.description = "Calculates (per-element) absolute difference between two images: |x - y|.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
    }
};

class NegateNodeType : public NodeType
{
public:
    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImage();

        // Do stuff
        if(src.type() == CV_8UC1)
            dst = cv::Scalar(255) - src;
        else if(src.type() == CV_8UC3)
            dst = cv::Scalar(255, 255, 255, 0) - src;

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::Image, "source", "Source", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Image, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        nodeConfig.description = "Negates image.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
    }
};

class CountNonZeroNodeType : public NodeType
{
public:
    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter&) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();

        // Do stuff
        int n = cv::countNonZero(src);

        return ExecutionStatus(EStatus::Ok, string_format("Non-zero elements: %d\n", n));
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::ImageMono, "source", "Source", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        nodeConfig.description = "Count non-zero pixel elements in given image.";
        nodeConfig.pInputSockets = in_config;
    }
};

REGISTER_NODE("Arithmetic/Count non-zero", CountNonZeroNodeType)
REGISTER_NODE("Arithmetic/Negate", NegateNodeType)
REGISTER_NODE("Arithmetic/Absolute diff.", AbsoluteDifferenceNodeType)
REGISTER_NODE("Arithmetic/Subtract", SubtractNodeType)
//REGISTER_NODE("Arithmetic/Absolute", AbsoluteNodeType)
REGISTER_NODE("Arithmetic/Add", AddNodeType)
