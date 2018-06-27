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

class CannyEdgeDetectorNodeType : public NodeType
{
public:
    CannyEdgeDetectorNodeType() : _threshold(10), _ratio(3)
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

        if (input.rows == 0 || input.cols == 0)
            return ExecutionStatus(EStatus::Ok);
        ;

        cv::Canny(input, output, _threshold, _threshold * _ratio, 3);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<double> _threshold;
    TypedNodeProperty<double> _ratio;
};

REGISTER_NODE("Features/Canny edge detector", CannyEdgeDetectorNodeType)
