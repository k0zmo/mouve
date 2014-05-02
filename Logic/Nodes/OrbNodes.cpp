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

#include <opencv2/features2d/features2d.hpp>

class OrbFeatureDetectorNodeType : public NodeType
{
public:
    OrbFeatureDetectorNodeType()
        : _nfeatures(1000)
        , _scaleFactor(1.2f)
        , _nlevels(8)
        , _edgeThreshold(31)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::NumFeatures:
            _nfeatures = newValue.toInt();
            return true;
        case pid::ScaleFactor:
            _scaleFactor = newValue.toFloat();
            return true;
        case pid::NumLevels:
            _nlevels = newValue.toInt();
            return true;
        case pid::EdgeThreshold:
            _edgeThreshold = newValue.toInt();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::NumFeatures: return _nfeatures;
        case pid::ScaleFactor: return _scaleFactor;
        case pid::NumLevels: return _nlevels;
        case pid::EdgeThreshold: return _edgeThreshold;
        }

        return NodeProperty();
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
        cv::OrbFeatureDetector orb(_nfeatures, _scaleFactor, _nlevels, _edgeThreshold);
        orb.detect(src, kp.kpoints);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok, 
            string_format("Keypoints detected: %d", (int) kp.kpoints.size()));
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::ImageMono, "image", "Image", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Integer, "Number of features to retain", "min:1" },
            { EPropertyType::Double, "Pyramid decimation ratio", "min:1.0" },
            { EPropertyType::Integer, "The number of pyramid levels", "min:1" },
            { EPropertyType::Integer, "Border margin", "min:1" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Extracts keypoints using FAST in pyramids.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
    }

protected:
    enum class pid
    {
        NumFeatures,
        ScaleFactor,
        NumLevels,
        EdgeThreshold
    };

    int _nfeatures;
    float _scaleFactor;
    int _nlevels;
    int _edgeThreshold;
};

class OrbDescriptorExtractorNodeType : public NodeType
{
public:
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
        cv::OrbDescriptorExtractor orb;
        orb.compute(kp.image, outKp.kpoints, outDescriptors);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Keypoints, "output", "Keypoints", "" },
            { ENodeFlowDataType::Array, "output", "Descriptors", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        nodeConfig.description = "Computes descriptors using oriented and rotated BRIEF (ORB).";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
    }
};

class OrbNodeType : public OrbFeatureDetectorNodeType
{
public:

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

        // Do stuff
        cv::ORB orb(_nfeatures, _scaleFactor, _nlevels, _edgeThreshold);
        orb(src, cv::noArray(), kp.kpoints, descriptors);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok, 
            string_format("Keypoints detected: %d", (int) kp.kpoints.size()));
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        OrbFeatureDetectorNodeType::configuration(nodeConfig);

        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
            { ENodeFlowDataType::Array, "output", "Descriptors", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        nodeConfig.description = "Detects keypoints and computes descriptors using oriented and rotated BRIEF (ORB).";
        nodeConfig.pOutputSockets = out_config;
    }
};

REGISTER_NODE("Features/Descriptors/ORB", OrbDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/ORB", OrbFeatureDetectorNodeType)
REGISTER_NODE("Features/ORB", OrbNodeType)
