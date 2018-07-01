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

class DrawMatchesNodeType : public NodeType
{
public:
    DrawMatchesNodeType() : _ecolor(EColor::AllRandom)
    {
        addInput("Matches", ENodeFlowDataType::Matches);
        addOutput("Image", ENodeFlowDataType::ImageRgb);
        addProperty("Keypoints color", _ecolor)
            .setUiHints("item: Random, item: Red, item: Green, item: Blue");
        setDescription("Draws matches.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const Matches& mt = reader.readSocket(0).getMatches();
        // Acquire output sockets
        cv::Mat& imageMatches = writer.acquireSocket(0).getImageRgb();

        // Validate inputs
        if (mt.queryImage.empty() || mt.trainImage.empty())
            return ExecutionStatus(EStatus::Ok);

        if (mt.queryPoints.size() != mt.trainPoints.size())
            return ExecutionStatus(
                EStatus::Error,
                "Points from one images doesn't correspond to key points in another one");

        const auto colorName = _ecolor.cast<Enum>().cast<EColor>();
        imageMatches = drawMatches(mt, colorName, getPairedColor(colorName));

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<EColor> _ecolor;
};

REGISTER_NODE("Draw features/Draw matches", DrawMatchesNodeType)
