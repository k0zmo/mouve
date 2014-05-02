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
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        if(propId > underlying_cast(pid::PatternScale) || 
            propId < underlying_cast(pid::Threshold))
            return false;

        switch(static_cast<pid>(propId))
        {
        case pid::Threshold:
            _thresh = newValue.toInt();
            break;
        case pid::NumOctaves:
            _nOctaves = newValue.toInt();
            break;
        case pid::PatternScale:
            _patternScale  = newValue.toFloat();
            break;
        }

        // That's a bit cheating here - creating BRISK object takes time (approx. 200ms for PAL)
        // which if done per frame makes it slowish (more than SIFT actually)
        _brisk = unique_ptr<cv::BRISK>(new cv::BRISK(_thresh, _nOctaves, _patternScale));
        return true;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Threshold: return _thresh;
        case pid::NumOctaves: return _nOctaves;
        case pid::PatternScale: return _patternScale;
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

        _brisk->detect(src, kp.kpoints);
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
            { EPropertyType::Integer, "FAST/AGAST detection threshold score", "min:1" },
            { EPropertyType::Integer, "Number of octaves", "min:0" },
            { EPropertyType::Double, "Pattern scale", "min:0.0" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
    }

protected:
    enum class pid
    {
        Threshold,
        NumOctaves,
        PatternScale
    };

    int _thresh;
    int _nOctaves;
    float _patternScale;
    // Must be pointer since BRISK doesn't implement copy/move operator (they should have)
    unique_ptr<cv::BRISK> _brisk;
};

class BriskDescriptorExtractorNodeType : public NodeType
{
public:
    BriskDescriptorExtractorNodeType()
        : _brisk(new cv::BRISK())
    {
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

        nodeConfig.description = "";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
    }

private:
    // Must be pointer since BRISK doesn't implement copy/move operator (they should have)
    unique_ptr<cv::BRISK> _brisk;
};

class BriskNodeType : public BriskFeatureDetectorNodeType
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

        (*_brisk)(src, cv::noArray(), kp.kpoints, descriptors);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok, 
            string_format("Keypoints detected: %d", (int) kp.kpoints.size()));
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        BriskFeatureDetectorNodeType::configuration(nodeConfig);

        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Keypoints, "keypoints", "Keypoints", "" },
            { ENodeFlowDataType::Array, "output", "Descriptors", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        nodeConfig.description = "";
        nodeConfig.pOutputSockets = out_config;
    }
};

REGISTER_NODE("Features/Descriptors/BRISK", BriskDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/BRISK", BriskFeatureDetectorNodeType)
REGISTER_NODE("Features/BRISK", BriskNodeType)
#endif