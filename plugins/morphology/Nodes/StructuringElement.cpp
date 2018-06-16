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

#include "Utils.h"

class StructuringElementNodeType : public NodeType
{
public:
    StructuringElementNodeType()
        : _se(EStructuringElementShape::Ellipse), _xradius(1), _yradius(1), _rotation(0)
    {
        addOutput("Structuring element", ENodeFlowDataType::ImageMono);
        addProperty("SE shape", _se)
            .setUiHints("item: Rectangle, item: Ellipse, item: Cross");
        addProperty("Horizontal radius", _xradius)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 50))
            .setUiHints("min:1, max:50");
        addProperty("Vertical radius", _yradius)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 50))
            .setUiHints("min:1, max:50");
        addProperty("Rotation", _rotation)
            .setUiHints("min:0, max:359, wrap:true");
        setDescription(
            "Generates a structuring element for a morphological "
            "operations with a given parameters describing its shape, size and rotation.");
    }

    ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
    {
        cv::Mat& kernel = writer.acquireSocket(0).getImageMono();

        if (_xradius == 0 || _yradius == 0)
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        kernel = standardStructuringElement(
            _xradius, _yradius, _se.cast<Enum>().cast<EStructuringElementShape>(), _rotation);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<EStructuringElementShape> _se;
    TypedNodeProperty<int> _xradius;
    TypedNodeProperty<int> _yradius;
    TypedNodeProperty<int> _rotation;
};

void registerStructuringElement(NodeSystem& system)
{
    system.registerNodeType("Morphology/Structuring element",
                            makeDefaultNodeFactory<StructuringElementNodeType>());
}
