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
#include "Logic/NodeSystem.h"

#include <opencv2/imgproc/imgproc.hpp>

class MorphologyOperatorNodeType : public NodeType
{
public:
    MorphologyOperatorNodeType() : _op(EMorphologyOperation::Erode)
    {
        addInput("Source", ENodeFlowDataType::Image);
        addInput("Structuring element", ENodeFlowDataType::ImageMono);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Operation type", _op)
            .setUiHints("item: Erode, item: Dilate, item: Open, item: Close,"
                        "item: Gradient, item: Top Hat, item: Black Hat");
        setDescription("Performs morphological operation on a given image.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImage();
        const cv::Mat& se = reader.readSocket(1).getImageMono();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImage();

        // Validate inputs
        if (se.empty() || src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::morphologyEx(src, dst, _op.cast<Enum>().data(), se);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    enum class EMorphologyOperation
    {
        Erode = cv::MORPH_ERODE,
        Dilate = cv::MORPH_DILATE,
        Open = cv::MORPH_OPEN,
        Close = cv::MORPH_CLOSE,
        Gradient = cv::MORPH_GRADIENT,
        TopHat = cv::MORPH_TOPHAT,
        BlackHat = cv::MORPH_BLACKHAT
    };

    TypedNodeProperty<EMorphologyOperation> _op;
};

void registerMorphologyOperator(NodeSystem& system)
{
    system.registerNodeType("Morphology/Operator",
                            makeDefaultNodeFactory<MorphologyOperatorNodeType>());
}
