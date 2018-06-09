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

#include "Logic/NodePlugin.h"
#include "Logic/NodeSystem.h"

#include "brisk.h"

#include <fmt/core.h>

class BriskFeatureDetectorNodeType : public NodeType
{
public:
    BriskFeatureDetectorNodeType()
        : _thresh(60)
        , _nOctaves(4)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addProperty("FAST/AGAST detection threshold score", _thresh)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Number of octaves", _nOctaves)
            .setValidator(make_validator<MinPropertyValidator<int>>(0))
            .setUiHints("min:0");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // inputs
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // outputs
        KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

        // validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        cv::BriskFeatureDetector brisk(_thresh, _nOctaves);
        brisk.detect(src, kp.kpoints);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Keypoints detected: {}", kp.kpoints.size()));
    }

private:
    TypedNodeProperty<int> _thresh;
    TypedNodeProperty<int> _nOctaves;
};

class BriskDescriptorExtractorNodeType : public NodeType
{
public:
    BriskDescriptorExtractorNodeType()
        : _rotationInvariant(true)
        , _scaleInvariant(true)
        , _patternScale(1.0f)
        , _brisk(nullptr)
    {
        addInput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        addProperty("Rotation invariant", _rotationInvariant)
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty&) { _brisk = nullptr; }));
        addProperty("Scale invariant", _scaleInvariant)
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty&) { _brisk = nullptr; }));
        addProperty("Pattern scale", _patternScale)
            .setValidator(make_validator<MinPropertyValidator<double>>(0.0))
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty&) { _brisk = nullptr; }))
            .setUiHints("min:0.0");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // inputs
        const KeyPoints& kp = reader.readSocket(0).getKeypoints();

        // validate inputs
        if(kp.kpoints.empty() || kp.image.empty())
            return ExecutionStatus(EStatus::Ok);

        if(!_brisk)
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

class BriskNodeType : public NodeType
{
public:
    BriskNodeType()
        : _thresh(60)
        , _nOctaves(4)
        , _rotationInvariant(true)
        , _scaleInvariant(true)
        , _patternScale(1.0f)
        , _brisk(nullptr)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        addProperty("FAST/AGAST detection threshold score", _thresh)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Number of octaves", _nOctaves)
            .setValidator(make_validator<MinPropertyValidator<int>>(0))
            .setUiHints("min:0");
        addProperty("Rotation invariant", _rotationInvariant)
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty&) { _brisk = nullptr; }));
        addProperty("Scale invariant", _scaleInvariant)
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty&) { _brisk = nullptr; }));
        addProperty("Pattern scale", _patternScale)
            .setValidator(make_validator<MinPropertyValidator<double>>(0.0))
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty&) { _brisk = nullptr; }))
            .setUiHints("min:0.0");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // inputs
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // ouputs
        KeyPoints& kp = writer.acquireSocket(0).getKeypoints();
        cv::Mat& descriptors = writer.acquireSocket(1).getArray();

        // validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        if(!_brisk)
        {
            _brisk = std::unique_ptr<cv::BriskDescriptorExtractor>(new cv::BriskDescriptorExtractor(
                _rotationInvariant, _scaleInvariant, _patternScale));
        }

        cv::BriskFeatureDetector _briskFeatureDetector(_thresh, _nOctaves);
        _briskFeatureDetector.detect(src, kp.kpoints);
        _brisk->compute(src, kp.kpoints, descriptors);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Keypoints detected: {}", kp.kpoints.size()));
    }

private:
    TypedNodeProperty<int> _thresh;
    TypedNodeProperty<int> _nOctaves;
    TypedNodeProperty<bool> _rotationInvariant;
    TypedNodeProperty<bool> _scaleInvariant;
    TypedNodeProperty<float> _patternScale;

    // Must be pointer since BriskDescriptorExtractor doesn't implement copy/move operator
    // and we want to cache _brisk object
    std::unique_ptr<cv::BriskDescriptorExtractor> _brisk;
};

extern "C" K_DECL_EXPORT int logicVersion()
{
    return LOGIC_VERSION;
}

extern "C" K_DECL_EXPORT int pluginVersion()
{
    return 1;
}

extern "C" K_DECL_EXPORT void registerPlugin(NodeSystem* nodeSystem)
{
    typedef DefaultNodeFactory<BriskFeatureDetectorNodeType> BriskFeatureDetectorFactory;
    typedef DefaultNodeFactory<BriskDescriptorExtractorNodeType> BriskDescriptorExtractorFactory;
    typedef DefaultNodeFactory<BriskNodeType> BriskFactory;

    nodeSystem->registerNodeType("Features/Detectors/BRISK", 
        std::unique_ptr<NodeFactory>(new BriskFeatureDetectorFactory()));
    nodeSystem->registerNodeType("Features/Descriptors/BRISK", 
        std::unique_ptr<NodeFactory>(new BriskDescriptorExtractorFactory()));
    nodeSystem->registerNodeType("Features/BRISK",
        std::unique_ptr<NodeFactory>(new BriskFactory()));
}