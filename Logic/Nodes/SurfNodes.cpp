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

#include "Logic/NodeType.h"
#include "Logic/NodeFactory.h"
#include "Kommon/StringUtils.h"

#include <opencv2/nonfree/features2d.hpp>

class SurfFeatureDetectorNodeType : public NodeType
{
public:
    SurfFeatureDetectorNodeType()
        : _hessianThreshold(400.0)
        , _nOctaves(4)
        , _nOctaveLayers(2)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addProperty("Hessian threshold", _hessianThreshold);
        addProperty("Number of octaves", _nOctaves)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Number of octave layers", _nOctaveLayers)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        setDescription("Extracts Speeded Up Robust Features from an image.");
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

        // Do stuff
        cv::SurfFeatureDetector detector(_hessianThreshold, _nOctaves, _nOctaveLayers);
        detector.detect(src, kp.kpoints);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok, 
            string_format("Keypoints detected: %d", (int) kp.kpoints.size()));
    }

private:
    TypedNodeProperty<double> _hessianThreshold;
    TypedNodeProperty<int> _nOctaves;
    TypedNodeProperty<int> _nOctaveLayers;
};

class SurfDescriptorExtractorNodeType : public NodeType
{
public:
    SurfDescriptorExtractorNodeType()
        : _extended(false)
        , _upright(false)
    {
        addInput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        addProperty("Extended", _extended);
        addProperty("Upright", _upright);
        setDescription("Describes local features using SURF algorithm.");
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

        // Do stuff
        cv::SurfDescriptorExtractor extractor;
        extractor.compute(kp.image, outKp.kpoints, outDescriptors);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<bool> _extended;
    TypedNodeProperty<bool> _upright;
};

class SurfNodeType : public NodeType
{
public:
    SurfNodeType()
        : _hessianThreshold(400.0)
        , _nOctaves(4)
        , _nOctaveLayers(2)
        , _extended(false)
        , _upright(false)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        addProperty("Hessian threshold", _hessianThreshold);
        addProperty("Number of octaves", _nOctaves)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Number of octave layers", _nOctaveLayers)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Extended", _extended);
        addProperty("Upright", _upright);
        setDescription("Extracts Speeded Up Robust Features and computes their descriptors from an image.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        KeyPoints& kp = writer.acquireSocket(0).getKeypoints();
        cv::Mat& descriptors = writer.acquireSocket(1).getArray();

        // Validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::SURF surf(_hessianThreshold, _nOctaves,
            _nOctaveLayers, _extended, _upright);
        surf(src, cv::noArray(), kp.kpoints, descriptors);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok, 
            string_format("Keypoints detected: %d", (int) kp.kpoints.size()));
    }

private:
    TypedNodeProperty<double> _hessianThreshold;
    TypedNodeProperty<int> _nOctaves;
    TypedNodeProperty<int> _nOctaveLayers;
    TypedNodeProperty<bool> _extended;
    TypedNodeProperty<bool> _upright;
};

REGISTER_NODE("Features/Descriptors/SURF", SurfDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/SURF", SurfFeatureDetectorNodeType)
REGISTER_NODE("Features/SURF", SurfNodeType)
