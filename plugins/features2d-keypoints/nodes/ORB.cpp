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
#include <fmt/core.h>

class OrbFeatureDetectorNodeType : public NodeType
{
public:
    OrbFeatureDetectorNodeType()
        : _nfeatures(1000), _scaleFactor(1.2f), _nlevels(8), _edgeThreshold(31)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addProperty("Number of features to retain", _nfeatures)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Pyramid decimation ratio", _scaleFactor)
            .setValidator(make_validator<MinPropertyValidator<double>>(1.0))
            .setUiHints("min:1.0");
        addProperty("The number of pyramid levels", _nlevels)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Border margin", _edgeThreshold)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        setDescription("Extracts keypoints using FAST in pyramids.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

        // Validate inputs
        if (src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::OrbFeatureDetector orb(_nfeatures, _scaleFactor, _nlevels, _edgeThreshold);
        orb.detect(src, kp.kpoints);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Keypoints detected: {}", kp.kpoints.size()));
    }

protected:
    TypedNodeProperty<int> _nfeatures;
    TypedNodeProperty<float> _scaleFactor;
    TypedNodeProperty<int> _nlevels;
    TypedNodeProperty<int> _edgeThreshold;
};

class OrbDescriptorExtractorNodeType : public NodeType
{
public:
    OrbDescriptorExtractorNodeType()
    {
        addInput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        setDescription("Computes descriptors using oriented and rotated BRIEF (ORB).");
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
        cv::OrbDescriptorExtractor orb;
        orb.compute(kp.image, outKp.kpoints, outDescriptors);

        return ExecutionStatus(EStatus::Ok);
    }
};

class OrbNodeType : public OrbFeatureDetectorNodeType
{
public:
    OrbNodeType() : OrbFeatureDetectorNodeType()
    {
        clearOutputs();
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        setDescription("Detects keypoints and computes descriptors "
                       "using oriented and rotated BRIEF (ORB).");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // ouputs
        KeyPoints& kp = writer.acquireSocket(0).getKeypoints();
        cv::Mat& descriptors = writer.acquireSocket(1).getArray();

        // Validate inputs
        if (src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::ORB orb(_nfeatures, _scaleFactor, _nlevels, _edgeThreshold);
        orb(src, cv::noArray(), kp.kpoints, descriptors);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Keypoints detected: {}", kp.kpoints.size()));
    }
};

REGISTER_NODE("Features/Descriptors/ORB", OrbDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/ORB", OrbFeatureDetectorNodeType)
REGISTER_NODE("Features/ORB", OrbNodeType)
