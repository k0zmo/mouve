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

// OpenCV BRISK uses different agast score - worse results
#if 0
#include "Logic/NodeType.h"
#include "Logic/NodeFactory.h"

#include "Kommon/StringUtils.h"

#include <opencv2/features2d/features2d.hpp>
using std::unique_ptr;

class BriskFeatureDetectorNodeType : public NodeType
{
public:
    BriskFeatureDetectorNodeType()
        : _thresh(30)
        , _nOctaves(3)
        , _patternScale(1.0f)
        , _brisk(new cv::BRISK())
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addProperty("FAST/AGAST detection threshold score", _thresh)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            // That's a bit cheating here - creating BRISK object takes time (approx. 200ms for PAL)
            // which if done per frame makes it slowish (more than SIFT actually)
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty&) { recreateBrisk(); }))
            .setUiHints("min:1");
        addProperty("Number of octaves", _nOctaves)
            .setValidator(make_validator<MinPropertyValidator<int>>(0))
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty&) { recreateBrisk(); }))
            .setUiHints("min:0");
        addProperty("Pattern scale", _patternScale)
            .setValidator(make_validator<MinPropertyValidator<double>>(0.0))
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty&) { recreateBrisk(); }))
            .setUiHints("min:0.0");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

        // Validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        _brisk->detect(src, kp.kpoints);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok, 
            string_format("Keypoints detected: %d", (int) kp.kpoints.size()));
    }

protected:
    void recreateBrisk()
    {
        _brisk = unique_ptr<cv::BRISK>(
            new cv::BRISK(_thresh, _nOctaves, _patternScale));
    }

protected:
    TypedNodeProperty<int> _thresh;
    TypedNodeProperty<int> _nOctaves;
    TypedNodeProperty<float> _patternScale;
    // Must be pointer since BRISK doesn't implement copy/move operator (they should have)
    unique_ptr<cv::BRISK> _brisk;
};

class BriskDescriptorExtractorNodeType : public NodeType
{
public:
    BriskDescriptorExtractorNodeType()
        : _brisk(new cv::BRISK())
    {
        addInput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const KeyPoints& kp = reader.readSocket(0).getKeypoints();

        // Validate inputs
        if(kp.kpoints.empty() || kp.image.empty())
            return ExecutionStatus(EStatus::Ok);

        // Acquire output sockets
        KeyPoints& outKp = writer.acquireSocket(0).getKeypoints();
        cv::Mat& outDescriptors = writer.acquireSocket(1).getArray();
        outKp = kp;

        _brisk->compute(kp.image, outKp.kpoints, outDescriptors);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    // Must be pointer since BRISK doesn't implement copy/move operator (they should have)
    unique_ptr<cv::BRISK> _brisk;
};

class BriskNodeType : public BriskFeatureDetectorNodeType
{
public:
    BriskNodeType()
    {
        clearOutputs();
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // ouputs
        KeyPoints& kp = writer.acquireSocket(0).getKeypoints();
        cv::Mat& descriptors = writer.acquireSocket(1).getArray();

        // Validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        (*_brisk)(src, cv::noArray(), kp.kpoints, descriptors);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok, 
            string_format("Keypoints detected: %d", (int) kp.kpoints.size()));
    }
};

REGISTER_NODE("Features/Descriptors/BRISK", BriskDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/BRISK", BriskFeatureDetectorNodeType)
REGISTER_NODE("Features/BRISK", BriskNodeType)
#endif