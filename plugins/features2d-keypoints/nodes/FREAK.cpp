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

#include <opencv2/features2d/features2d.hpp>

class FreakDescriptorExtractorNodeType : public NodeType
{
public:
    FreakDescriptorExtractorNodeType()
        : _orientationNormalized(true), _scaleNormalized(true), _patternScale(22.0f), _nOctaves(4)
    {
        addInput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        addProperty("Enable orientation normalization", _orientationNormalized);
        addProperty("Enable scale normalization", _scaleNormalized);
        addProperty("Scaling of the description pattern", _patternScale)
            .setValidator(make_validator<MinPropertyValidator<float>>(1.0f))
            .setUiHints("min:1.0");
        addProperty("Number of octaves covered by detected keypoints", _nOctaves)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        setDescription("FREAK (Fast Retina Keypoint) keypoint descriptor.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const KeyPoints& kp = reader.readSocket(0).getKeypoints();

        // Validate inputs
        if (kp.kpoints.empty() || kp.image.empty())
            return ExecutionStatus(EStatus::Ok);

        // Acquire output sockets
        KeyPoints& outKp = writer.acquireSocket(0).getKeypoints();
        cv::Mat& outDescriptors = writer.acquireSocket(1).getArray();
        outKp = kp;

        // Do stuff
        cv::FREAK freak(_orientationNormalized, _scaleNormalized, _patternScale, _nOctaves);
        freak.compute(kp.image, outKp.kpoints, outDescriptors);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<bool> _orientationNormalized;
    TypedNodeProperty<bool> _scaleNormalized;
    TypedNodeProperty<float> _patternScale;
    TypedNodeProperty<int> _nOctaves;
};

REGISTER_NODE("Features/Descriptors/FREAK", FreakDescriptorExtractorNodeType)
