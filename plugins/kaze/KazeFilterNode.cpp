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

#include "Logic/NodePlugin.h"
#include "Logic/NodeType.h"
#include "Logic/NodeSystem.h"

#include "lib/KAZE.h"
#include "lib/AKAZE.h"

#include <fmt/core.h>

class KazeFeatureDetectorNodeType : public NodeType
{
public:
    KazeFeatureDetectorNodeType()
        : _nOctaves(4)
        , _nSubLevels(4)
        , _diffusivityType(libKAZE::PM_G2)
        , _detectorThreshold(0.0003f)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);

        addProperty("Maximum octave of image evolution", _nOctaves);
        addProperty("Number of sublevels per octave", _nSubLevels);
        addProperty("Diffusivity function", _diffusivityType)
            .setUiHints("item: Perona-Malik (g1),"
                        "item: Perona-Malik (g2),"
                        "item: Weickert diffusivity,"
                        "item: Charbonnier diffusivity");
        addProperty("Feature detector threshold response", _detectorThreshold)
            .setUiHints("decimals:4");
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

        cv::Mat srcFloat;
        src.convertTo(srcFloat, CV_32F, 1.0/255.0, 0);

        libKAZE::KAZEOptions options;
        options.img_width = src.cols;
        options.img_height = src.rows;
        options.omax = _nOctaves;
        options.nsublevels = _nSubLevels;
        options.diffusivity = _diffusivityType.toEnum().cast<libKAZE::DIFFUSIVITY_TYPE>();
        options.dthreshold = _detectorThreshold;

        kp.kpoints.clear();
        libKAZE::KAZE evolution(options);
        evolution.Create_Nonlinear_Scale_Space(srcFloat);
        evolution.Feature_Detection(kp.kpoints);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Keypoints detected: {}", kp.kpoints.size()));
    }

private:
    TypedNodeProperty<int> _nOctaves;
    TypedNodeProperty<int> _nSubLevels;
    TypedNodeProperty<libKAZE::DIFFUSIVITY_TYPE> _diffusivityType;
    TypedNodeProperty<float> _detectorThreshold;
};

enum class EKazeDescriptor
{
    SURF,
    MSURF,
    GSURF
};

class KazeNodeType : public NodeType
{
public:
    KazeNodeType()
        : _nOctaves(4)
        , _nSubLevels(4)
        , _diffusivityType(libKAZE::PM_G2)
        , _detectorThreshold(0.0003f)
        , _upright(false)
        , _extendedDescriptor(false)
        , _descriptorType(EKazeDescriptor::MSURF)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        addProperty("Maximum octave of image evolution", _nOctaves);
        addProperty("Number of sublevels per octave", _nSubLevels);
        addProperty("Diffusivity function", _diffusivityType)
            .setUiHints("item: Perona-Malik (g1),"
                        "item: Perona-Malik (g2),"
                        "item: Weickert diffusivity,"
                        "item: Charbonnier diffusivity");
        addProperty("Feature detector threshold response", _detectorThreshold)
            .setUiHints("decimals:4");
        addProperty("Upright", _upright);
        addProperty("Extended descriptor", _extendedDescriptor);
        addProperty("Descriptor type", _descriptorType)
            .setUiHints("item: SURF, item: MSURF, item: GSURF");
        setDescription(
            "Extracts features using KAZE algorithm and computes their descriptors from an image.");
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

        cv::Mat srcFloat;
        src.convertTo(srcFloat, CV_32F, 1.0/255.0, 0);

        libKAZE::KAZEOptions options;
        options.img_width = src.cols;
        options.img_height = src.rows;
        options.omax = _nOctaves;
        options.nsublevels = _nSubLevels;
        options.diffusivity = _diffusivityType.toEnum().cast<libKAZE::DIFFUSIVITY_TYPE>();
        options.dthreshold = _detectorThreshold;
        options.descriptor = [this]() {
            using namespace libKAZE;
            switch (_descriptorType.toEnum().cast<EKazeDescriptor>())
            {
            case EKazeDescriptor::SURF:
                if (_upright)
                    return _extendedDescriptor ? SURF_EXTENDED_UPRIGHT : SURF_UPRIGHT;
                return _extendedDescriptor ? SURF_EXTENDED : SURF;
            case EKazeDescriptor::MSURF:
                if (_upright)
                    return _extendedDescriptor ? MSURF_EXTENDED_UPRIGHT : MSURF_UPRIGHT;
                return _extendedDescriptor ? MSURF_EXTENDED : MSURF;
            case EKazeDescriptor::GSURF:
                if (_upright)
                    return _extendedDescriptor ? GSURF_EXTENDED_UPRIGHT : GSURF_UPRIGHT;
                return _extendedDescriptor ? GSURF_EXTENDED : GSURF;
            }
            return MSURF;
        }();

        kp.kpoints.clear();
        libKAZE::KAZE evolution(options);
        evolution.Create_Nonlinear_Scale_Space(srcFloat);
        evolution.Feature_Detection(kp.kpoints);
        evolution.Compute_Descriptors(kp.kpoints, descriptors);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Keypoints detected: {}", kp.kpoints.size()));
    }

private:
    TypedNodeProperty<int> _nOctaves;
    TypedNodeProperty<int> _nSubLevels;
    TypedNodeProperty<libKAZE::DIFFUSIVITY_TYPE> _diffusivityType;
    TypedNodeProperty<float> _detectorThreshold;
    TypedNodeProperty<bool> _upright;
    TypedNodeProperty<bool> _extendedDescriptor;
    TypedNodeProperty<EKazeDescriptor> _descriptorType;
};

class AkazeFeatureDetectorNodeType : public NodeType
{
public:
    AkazeFeatureDetectorNodeType()
        : _nOctaves(4)
        , _nSubLevels(4)
        , _diffusivityType(libAKAZE::PM_G2)
        , _detectorThreshold(0.001f)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);

        addProperty("Maximum octave of image evolution", _nOctaves);
        addProperty("Number of sublevels per octave", _nSubLevels);
        addProperty("Diffusivity function", _diffusivityType)
            .setUiHints("item: Perona-Malik (g1),"
                        "item: Perona-Malik (g2),"
                        "item: Weickert diffusivity,"
                        "item: Charbonnier diffusivity");
        addProperty("Feature detector threshold response", _detectorThreshold)
            .setUiHints("decimals:4");
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

        cv::Mat srcFloat;
        src.convertTo(srcFloat, CV_32F, 1.0/255.0, 0);

        libAKAZE::AKAZEOptions options;
        options.img_width = src.cols;
        options.img_height = src.rows;
        options.omax = _nOctaves;
        options.nsublevels = _nSubLevels;
        options.diffusivity = _diffusivityType.toEnum().cast<libAKAZE::DIFFUSIVITY_TYPE>();
        options.dthreshold = _detectorThreshold;

        kp.kpoints.clear();
        libAKAZE::AKAZE evolution(options);
        evolution.Create_Nonlinear_Scale_Space(srcFloat);
        evolution.Feature_Detection(kp.kpoints);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Keypoints detected: {}", kp.kpoints.size()));
    }

private:
    TypedNodeProperty<int> _nOctaves;
    TypedNodeProperty<int> _nSubLevels;
    TypedNodeProperty<libAKAZE::DIFFUSIVITY_TYPE> _diffusivityType;
    TypedNodeProperty<float> _detectorThreshold;
};

enum class EAkazeDescriptor
{
    SURF,
    MSURF,
    MLDB // Modified Local Difference Binary
};

class AkazeNodeType : public NodeType
{
public:
    AkazeNodeType()
        : _nOctaves(4)
        , _nSubLevels(4)
        , _diffusivityType(libAKAZE::PM_G2)
        , _detectorThreshold(0.001f)
        , _upright(false)
        , _descriptorType(EAkazeDescriptor::MLDB)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Descriptors", ENodeFlowDataType::Array);
        addProperty("Maximum octave of image evolution", _nOctaves);
        addProperty("Number of sublevels per octave", _nSubLevels);
        addProperty("Diffusivity function", _diffusivityType)
            .setUiHints("item: Perona-Malik (g1),"
                        "item: Perona-Malik (g2),"
                        "item: Weickert diffusivity,"
                        "item: Charbonnier diffusivity");
        addProperty("Feature detector threshold response", _detectorThreshold)
            .setUiHints("decimals:4");
        addProperty("Upright", _upright);
        addProperty("Descriptor type", _descriptorType)
            .setUiHints("item: SURF, item: MSURF, item: MLDB");
        setDescription(
            "Extracts features using AKAZE algorithm and computes their descriptors from an image.");
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

        cv::Mat srcFloat;
        src.convertTo(srcFloat, CV_32F, 1.0/255.0, 0);

        libAKAZE::AKAZEOptions options;
        options.img_width = src.cols;
        options.img_height = src.rows;
        options.omax = _nOctaves;
        options.nsublevels = _nSubLevels;
        options.diffusivity = _diffusivityType.toEnum().cast<libAKAZE::DIFFUSIVITY_TYPE>();
        options.dthreshold = _detectorThreshold;
        options.descriptor = [this]() {
            using namespace libAKAZE;
            switch (_descriptorType.toEnum().cast<EAkazeDescriptor>())
            {
            case EAkazeDescriptor::SURF:
                return _upright ? SURF_UPRIGHT : SURF;
            case EAkazeDescriptor::MSURF:
                return _upright ? MSURF_UPRIGHT : MSURF;
            case EAkazeDescriptor::MLDB:
                return _upright ? MLDB_UPRIGHT : MLDB;
            }
            return MLDB;
        }();

        kp.kpoints.clear();
        libAKAZE::AKAZE evolution(options);
        evolution.Create_Nonlinear_Scale_Space(srcFloat);
        evolution.Feature_Detection(kp.kpoints);
        evolution.Compute_Descriptors(kp.kpoints, descriptors);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Keypoints detected: {}", kp.kpoints.size()));
    }

private:
    TypedNodeProperty<int> _nOctaves;
    TypedNodeProperty<int> _nSubLevels;
    TypedNodeProperty<libAKAZE::DIFFUSIVITY_TYPE> _diffusivityType;
    TypedNodeProperty<float> _detectorThreshold;
    TypedNodeProperty<bool> _upright;
    TypedNodeProperty<EAkazeDescriptor> _descriptorType;
};

class KazePlugin : public NodePlugin
{
    MOUVE_DECLARE_PLUGIN(1);

public:
    void registerPlugin(NodeSystem& system) override
    {
        system.registerNodeType("Features/Detectors/KAZE",
                                makeDefaultNodeFactory<KazeFeatureDetectorNodeType>());
        system.registerNodeType("Features/Detectors/AKAZE",
                                makeDefaultNodeFactory<AkazeFeatureDetectorNodeType>());
        // There's no separate descriptor extractor node type because (A)KAZE descriptors are
        // calculated using non-linear scale space which means we would need to build it anyway
        system.registerNodeType("Features/KAZE",
                                makeDefaultNodeFactory<KazeNodeType>());
        system.registerNodeType("Features/AKAZE",
                                makeDefaultNodeFactory<AkazeNodeType>());
    }
};

MOUVE_INSTANTIATE_PLUGIN(KazePlugin)
