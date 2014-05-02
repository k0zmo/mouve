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

#include "ksurf.h"

class kSurfFeatureDetectorNodeType : public NodeType
{
public:
    kSurfFeatureDetectorNodeType()
        : _hessianThreshold(35.0)
        , _nOctaves(4)
        , _nScales(4)
        , _initSampling(1)
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
        case pid::NumScales:
            _nScales = newValue.toInt();
            return true;
        case pid::InitSampling:
            _initSampling = newValue.toInt();
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
        case pid::NumScales: return _nScales;
        case pid::InitSampling: return _initSampling;
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
        kSURF surf(_hessianThreshold, _nOctaves, _nScales, _initSampling);
        surf(src, cv::noArray(), kp.kpoints);
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
            { EPropertyType::Integer, "Number of scale levels", "min:1" },
            { EPropertyType::Integer, "Initial sampling rate", "min:1" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Extracts Speeded Up Robust Features from an image.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
    }

protected:
    enum class pid
    {
        HessianThreshold,
        NumOctaves,
        NumScales,
        InitSampling,
    };

    double _hessianThreshold;
    int _nOctaves;
    int _nScales;
    int _initSampling;
};

class kSurfDescriptorExtractorNodeType : public NodeType
{
public:
    kSurfDescriptorExtractorNodeType()
        : _msurf(true)
        , _upright(false)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::MSurfDescriptor:
            _msurf = newValue.toBool();
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
        case pid::MSurfDescriptor: return _msurf;
        case pid::Upright: return _upright;
        }

        return NodeProperty();
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const KeyPoints& kp = reader.readSocket(0).getKeypoints();

        // valudate inputs
        if(kp.kpoints.empty() || kp.image.empty())
            return ExecutionStatus(EStatus::Ok);

        // Acquire output sockets
        KeyPoints& outKp = writer.acquireSocket(0).getKeypoints();
        cv::Mat& outDescriptors = writer.acquireSocket(1).getArray();
        outKp = kp;

        // Do stuff
        kSURF surf(1, 1, 1, 1, _msurf, _upright);
        surf.compute(kp.image, outKp.kpoints, outDescriptors);

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
            { EPropertyType::Boolean, "MSURF descriptor", "" },
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
        MSurfDescriptor,
        Upright
    };

    bool _msurf;
    bool _upright;
};

class kSurfNodeType : public NodeType
{
public:
    kSurfNodeType()
        : _hessianThreshold(35.0)
        , _nOctaves(4)
        , _nScales(4)
        , _initSampling(1)
        , _msurf(true)
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
        case pid::NumScales:
            _nScales = newValue.toInt();
            return true;
        case pid::InitSampling:
            _initSampling = newValue.toInt();
            return true;
        case pid::MSurfDescriptor:
            _msurf = newValue.toBool();
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
        case pid::NumScales: return _nScales;
        case pid::InitSampling: return _initSampling;
        case pid::MSurfDescriptor: return _msurf;
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
        kSURF surf(_hessianThreshold, _nOctaves, _nScales, _initSampling, _msurf, _upright);
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
            { EPropertyType::Integer, "Number of scale levels", "min:1" },
            { EPropertyType::Integer, "Initial sampling rate", "min:1" },
            { EPropertyType::Boolean, "MSURF descriptor", "" },
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
        NumScales,
        InitSampling,
        MSurfDescriptor,
        Upright
    };

    double _hessianThreshold;
    int _nOctaves;
    int _nScales;
    int _initSampling;
    bool _msurf;
    bool _upright;
};

REGISTER_NODE("Features/Descriptors/kSURF", kSurfDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/kSURF", kSurfFeatureDetectorNodeType)
REGISTER_NODE("Features/kSURF", kSurfNodeType)
