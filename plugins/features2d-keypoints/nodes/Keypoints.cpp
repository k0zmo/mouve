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

#include <fmt/core.h>

class RetainBestFeaturesNodeType : public NodeType
{
public:
    RetainBestFeaturesNodeType() : _n(1000)
    {
        addInput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Best", ENodeFlowDataType::Keypoints);
        addProperty("Best keypoints to retain", _n)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        setDescription(
            "Retains the specified number of the best keypoints (according to the response).");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const KeyPoints& src = reader.readSocket(0).getKeypoints();
        // Acquire output sockets
        KeyPoints& dst = writer.acquireSocket(0).getKeypoints();

        // Validate inputs
        if (src.kpoints.empty())
            return ExecutionStatus(EStatus::Ok);

        // Prepare output structure
        dst.image = src.image;
        dst.kpoints.clear();
        std::copy(std::begin(src.kpoints), std::end(src.kpoints), std::back_inserter(dst.kpoints));

        // Do stuff
        cv::KeyPointsFilter::retainBest(dst.kpoints, _n);

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Keypoints retained: {}", dst.kpoints.size()));
    }

private:
    TypedNodeProperty<int> _n;
};

REGISTER_NODE("Features/Retain best", RetainBestFeaturesNodeType)
