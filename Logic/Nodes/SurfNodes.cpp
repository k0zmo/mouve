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
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::HessianThreshold:
            _hessianThreshold = newValue.toDouble();
            return true;
        case pid::NumOctaves:
            _nOctaves = newValue.toInt();
            return true;
        case pid::NumOctaveLayers:
            _nOctaveLayers = newValue.toInt();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::HessianThreshold: return _hessianThreshold;
        case pid::NumOctaves: return _nOctaves;
        case pid::NumOctaveLayers: return _nOctaveLayers;
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
        cv::SurfFeatureDetector detector(_hessianThreshold, _nOctaves, _nOctaveLayers);
        detector.detect(src, kp.kpoints);
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
            { EPropertyType::Double, "Hessian threshold", "" },
            { EPropertyType::Integer, "Number of octaves", "min:1" },
            { EPropertyType::Integer, "Number of octave layers", "min:1" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Extracts Speeded Up Robust Features from an image.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
    }

private:
    enum class pid
    {
        HessianThreshold,
        NumOctaves,
        NumOctaveLayers
    };

    double _hessianThreshold;
    int _nOctaves;
    int _nOctaveLayers;
};

class SurfDescriptorExtractorNodeType : public NodeType
{
public:
    SurfDescriptorExtractorNodeType()
        : _extended(false)
        , _upright(false)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Extended:
            _extended = newValue.toBool();
            return true;
        case pid::Upright:
            _upright = newValue.toBool();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Extended: return _extended;
        case pid::Upright: return _upright;
        }

        return NodeProperty();
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
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Boolean, "Extended", "" },
            { EPropertyType::Boolean, "Upright", "" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Describes local features using SURF algorithm.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
    }

private:
    enum class pid
    {
        Extended,
        Upright
    };

    bool _extended;
    bool _upright;
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
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::HessianThreshold:
            _hessianThreshold = newValue.toDouble();
            return true;
        case pid::NumOctaves:
            _nOctaves = newValue.toInt();
            return true;
        case pid::NumOctaveLayers:
            _nOctaveLayers = newValue.toInt();
            return true;
        case pid::Extended:
            _extended = newValue.toBool();
            return true;
        case pid::Upright:
            _upright = newValue.toBool();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::HessianThreshold: return _hessianThreshold;
        case pid::NumOctaves: return _nOctaves;
        case pid::NumOctaveLayers: return _nOctaveLayers;
        case pid::Extended: return _extended;
        case pid::Upright: return _upright;
        }

        return NodeProperty();
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

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::ImageMono, "image", "Image", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
            { ENodeFlowDataType::Array, "output", "Descriptors", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Double, "Hessian threshold", "" },
            { EPropertyType::Integer, "Number of octaves", "min:1" },
            { EPropertyType::Integer, "Number of octave layers", "min:1" },
            { EPropertyType::Boolean, "Extended", "" },
            { EPropertyType::Boolean, "Upright", "" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Extracts Speeded Up Robust Features and computes their descriptors from an image.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
    }

private:
    enum class pid
    {
        HessianThreshold,
        NumOctaves,
        NumOctaveLayers,
        Extended,
        Upright
    };

    double _hessianThreshold;
    int _nOctaves;
    int _nOctaveLayers;
    bool _extended;
    bool _upright;
};

REGISTER_NODE("Features/Descriptors/SURF", SurfDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/SURF", SurfFeatureDetectorNodeType)
REGISTER_NODE("Features/SURF", SurfNodeType)
