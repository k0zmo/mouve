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

#include "impl/Drawing.h"

class DrawKeypointsNodeType : public NodeType
{
public:
    DrawKeypointsNodeType() : _ecolor(EColor::AllRandom), _richKeypoints(true)
    {
        addInput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Output", ENodeFlowDataType::ImageRgb);
        addProperty("Keypoints color", _ecolor)
            .setUiHints("item: Random, item: Red, item: Green, item: Blue");
        addProperty("Rich keypoints", _richKeypoints);
        setDescription("Draws keypoints.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const KeyPoints& kp = reader.readSocket(0).getKeypoints();
        // Acquire output sockets
        cv::Mat& imageDst = writer.acquireSocket(0).getImageRgb();

        // validate input
        if (kp.image.empty())
            return ExecutionStatus(EStatus::Ok);

        drawKeypoints(kp.image, kp.kpoints, imageDst, _ecolor.cast<Enum>().cast<EColor>(),
                      _richKeypoints);
        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<EColor> _ecolor;
    TypedNodeProperty<bool> _richKeypoints;
};

REGISTER_NODE("Draw features/Draw keypoints", DrawKeypointsNodeType)
