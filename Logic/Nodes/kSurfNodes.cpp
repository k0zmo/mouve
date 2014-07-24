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
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addProperty("Hessian threshold", _hessianThreshold);
        addProperty("Number of octaves", _nOctaves)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Number of scale levels", _nScales)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Initial sampling rate", _initSampling)
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
        kSURF surf(_hessianThreshold, _nOctaves, _nScales, _initSampling);
        surf(src, cv::noArray(), kp.kpoints);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok, 
            string_format("Keypoints detected: %d", (int) kp.kpoints.size()));
    }

protected:
    TypedNodeProperty<double> _hessianThreshold;
    TypedNodeProperty<int> _nOctaves;
    TypedNodeProperty<int> _nScales;
    TypedNodeProperty<int> _initSampling;
};

class kSurfDescriptorExtractorNodeType : public NodeType
{
public:
    kSurfDescriptorExtractorNodeType()
        : _msurf(true)
        , _upright(false)
    {
        addInput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        addProperty("MSURF descriptor", _msurf);
        addProperty("Upright", _upright);
        setDescription("Describes local features using SURF algorithm.");
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

private:
    TypedNodeProperty<bool> _msurf;
    TypedNodeProperty<bool> _upright;
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
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        addProperty("Hessian threshold", _hessianThreshold);
        addProperty("Number of octaves", _nOctaves)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Number of scale levels", _nScales)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("Initial sampling rate", _initSampling)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        addProperty("MSURF descriptor", _msurf);
        addProperty("Upright", _upright);
        setDescription("Extracts Speeded Up Robust Features and "
            "computes their descriptors from an image.");
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

private:
    TypedNodeProperty<double> _hessianThreshold;
    TypedNodeProperty<int> _nOctaves;
    TypedNodeProperty<int> _nScales;
    TypedNodeProperty<int> _initSampling;
    TypedNodeProperty<bool> _msurf;
    TypedNodeProperty<bool> _upright;
};

REGISTER_NODE("Features/Descriptors/kSURF", kSurfDescriptorExtractorNodeType)
REGISTER_NODE("Features/Detectors/kSURF", kSurfFeatureDetectorNodeType)
REGISTER_NODE("Features/kSURF", kSurfNodeType)
