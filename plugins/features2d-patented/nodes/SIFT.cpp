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

#include <opencv2/nonfree/features2d.hpp>
#include <fmt/core.h>

class SiftFeatureDetectorNodeType : public NodeType
{
public:
    SiftFeatureDetectorNodeType()
        : _nFeatures(0),
          _nOctaveLayers(3),
          _contrastThreshold(0.04),
          _edgeThreshold(10),
          _sigma(1.6)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addProperty("Number of best features to retain", _nFeatures)
            .setValidator(make_validator<MinPropertyValidator<int>>(0))
            .setUiHints("min:0");
        addProperty("Number of layers in each octave", _nOctaveLayers)
            .setValidator(make_validator<MinPropertyValidator<int>>(2))
            .setUiHints("min:2");
        addProperty("Contrast threshold", _contrastThreshold);
        addProperty("Edge threshold", _edgeThreshold);
        addProperty("Input image sigma", _sigma);
        setDescription("Extracts keypoints using difference of gaussians (DoG) algorithm.");
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
        cv::SiftFeatureDetector detector(_nFeatures, _nOctaveLayers, _contrastThreshold,
                                         _edgeThreshold, _sigma);
        detector.detect(src, kp.kpoints);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Keypoints detected: {}", kp.kpoints.size()));
    }

protected:
    TypedNodeProperty<int> _nFeatures;
    TypedNodeProperty<int> _nOctaveLayers;
    TypedNodeProperty<double> _contrastThreshold;
    TypedNodeProperty<double> _edgeThreshold;
    TypedNodeProperty<double> _sigma;
};

class SiftDescriptorExtractorNodeType : public NodeType
{
public:
    SiftDescriptorExtractorNodeType()
    {
        addInput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        setDescription(
            "Computes descriptors using the Scale Invariant Feature Transform (SIFT) algorithm.");
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
        cv::SiftDescriptorExtractor extractor;
        extractor.compute(kp.image, outKp.kpoints, outDescriptors);

        return ExecutionStatus(EStatus::Ok);
    }
};

class SiftNodeType : public SiftFeatureDetectorNodeType
{
public:
    SiftNodeType() : SiftFeatureDetectorNodeType()
    {
        // Just add one more output socket
        clearOutputs();
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        setDescription("Extracts keypoints and computes descriptors using "
                       "the Scale Invariant Feature Transform (SIFT) algorithm.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        KeyPoints& kp = writer.acquireSocket(0).getKeypoints();
        cv::Mat& descriptors = writer.acquireSocket(1).getArray();

        // Validate inputs
        if (src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::SIFT sift(_nFeatures, _nOctaveLayers, _contrastThreshold, _edgeThreshold, _sigma);
        sift(src, cv::noArray(), kp.kpoints, descriptors);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Keypoints detected: {}", kp.kpoints.size()));
    }
};

REGISTER_NODE("Features/Descriptors/SIFT", SiftDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/SIFT", SiftFeatureDetectorNodeType)
REGISTER_NODE("Features/SIFT", SiftNodeType)
