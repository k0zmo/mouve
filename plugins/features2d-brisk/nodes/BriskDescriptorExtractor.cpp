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

#include "impl/brisk.h"

#include <fmt/core.h>

class BriskDescriptorExtractorNodeType : public NodeType
{
public:
    BriskDescriptorExtractorNodeType()
        : _rotationInvariant(true), _scaleInvariant(true), _patternScale(1.0f), _brisk(nullptr)
    {
        addInput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        addProperty("Rotation invariant", _rotationInvariant)
            .setObserver(
                make_observer<FuncObserver>([this](const NodeProperty&) { _brisk = nullptr; }));
        addProperty("Scale invariant", _scaleInvariant)
            .setObserver(
                make_observer<FuncObserver>([this](const NodeProperty&) { _brisk = nullptr; }));
        addProperty("Pattern scale", _patternScale)
            .setValidator(make_validator<MinPropertyValidator<double>>(0.0))
            .setObserver(
                make_observer<FuncObserver>([this](const NodeProperty&) { _brisk = nullptr; }))
            .setUiHints("min:0.0");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // inputs
        const KeyPoints& kp = reader.readSocket(0).getKeypoints();

        // validate inputs
        if (kp.kpoints.empty() || kp.image.empty())
            return ExecutionStatus(EStatus::Ok);

        if (!_brisk)
        {
            _brisk = std::unique_ptr<cv::BriskDescriptorExtractor>(new cv::BriskDescriptorExtractor(
                _rotationInvariant, _scaleInvariant, _patternScale));
        }

        // outputs
        KeyPoints& outKp = writer.acquireSocket(0).getKeypoints();
        cv::Mat& outDescriptors = writer.acquireSocket(1).getArray();
        outKp = kp;

        _brisk->compute(kp.image, outKp.kpoints, outDescriptors);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<bool> _rotationInvariant;
    TypedNodeProperty<bool> _scaleInvariant;
    TypedNodeProperty<float> _patternScale;

    // Must be pointer since BriskDescriptorExtractor doesn't implement copy/move operator
    // and we want to cache _brisk object
    std::unique_ptr<cv::BriskDescriptorExtractor> _brisk;
};

REGISTER_NODE("Features/Descriptors/BRISK", BriskDescriptorExtractorNodeType);
