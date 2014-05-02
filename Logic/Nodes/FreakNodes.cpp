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

#include <opencv2/features2d/features2d.hpp>

class FreakDescriptorExtractorNodeType : public NodeType
{
public:
    FreakDescriptorExtractorNodeType()
        : _orientationNormalized(true)
        , _scaleNormalized(true)
        , _patternScale(22.0f)
        , _nOctaves(4)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::OrientationNormalized:
            _orientationNormalized = newValue.toBool();
            return true;
        case pid::ScaleNormalized:
            _scaleNormalized = newValue.toBool();
            return true;
        case pid::PatternScale:
            _patternScale = newValue.toFloat();
            return true;
        case pid::NumOctaves:
            _nOctaves = newValue.toInt();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::OrientationNormalized: return _orientationNormalized;
        case pid::ScaleNormalized: return _scaleNormalized;
        case pid::PatternScale: return _patternScale;
        case pid::NumOctaves: return _nOctaves;
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
        cv::FREAK freak(_orientationNormalized, _scaleNormalized, _patternScale, _nOctaves);
        freak.compute(kp.image, outKp.kpoints, outDescriptors);

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
            { EPropertyType::Boolean, "Enable orientation normalization", "" },
            { EPropertyType::Boolean, "Enable scale normalization", "" },
            { EPropertyType::Double, "Scaling of the description pattern", "min:1.0" },
            { EPropertyType::Integer, "Number of octaves covered by detected keypoints", "min:1" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "FREAK (Fast Retina Keypoint) keypoint descriptor.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
    }

private:
    enum class pid
    {
        OrientationNormalized,
        ScaleNormalized,
        PatternScale,
        NumOctaves,
    };

    bool _orientationNormalized;
    bool _scaleNormalized;
    float _patternScale;
    int _nOctaves;
};
REGISTER_NODE("Features/Descriptors/FREAK", FreakDescriptorExtractorNodeType)
