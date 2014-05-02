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

#include <opencv2/video/video.hpp>

#include "CV.h"

class MixtureOfGaussiansNodeType : public NodeType
{
public:
    MixtureOfGaussiansNodeType()
        : _history(200)
        , _nmixtures(5)
        , _backgroundRatio(0.7)
        , _learningRate(-1)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::History:
            _history = newValue.toInt();
            return true;
        case pid::NMixtures:
            _nmixtures = newValue.toInt();
            return true;
        case pid::BackgroundRatio:
            _backgroundRatio = newValue.toDouble();
            return true;
        case pid::LearningRate:
            _learningRate = newValue.toDouble();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::History: return _history;
        case pid::NMixtures: return _nmixtures;
        case pid::BackgroundRatio: return _backgroundRatio;
        case pid::LearningRate: return _learningRate;
        }

        return NodeProperty();
    }

    bool restart() override
    {
        _mog = cv::BackgroundSubtractorMOG(_history, _nmixtures, _backgroundRatio);
        return true;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& source = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImageMono();

        // Validate inputs
        if(source.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff - single step
        _mog(source, output, _learningRate);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::Image, "input", "Input", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::ImageMono, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        static const PropertyConfig prop_config[] = {
            { EPropertyType::Integer, "History frames", "min:1, max:500" },
            { EPropertyType::Integer, "Number of mixtures",  "min:1, max:9" },
            { EPropertyType::Double, "Background ratio", "min:0.01, max:0.99, step:0.01" },
            { EPropertyType::Double, "Learning rate", "min:-1, max:1, step:0.01, decimals:3" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Gaussian Mixture-based image sequence background/foreground segmentation.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
        nodeConfig.flags = ENodeConfig::HasState;
    }

private:
    enum class pid
    {
        History,
        NMixtures,
        BackgroundRatio,
        LearningRate
    };

    cv::BackgroundSubtractorMOG _mog;
    int _history;
    int _nmixtures;
    double _backgroundRatio;
    double _learningRate;
};

class AdaptiveMixtureOfGaussiansNodeType : public NodeType
{
public:
    AdaptiveMixtureOfGaussiansNodeType()
        : _learningRate(-1)
        , _history(200)
        , _varThreshold(16.0f)
        , _shadowDetection(true)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::History:
            _history = newValue.toInt();
            return true;
        case pid::VarianceThreshold:
            _varThreshold = newValue.toFloat();
            return true;
        case pid::ShadowDetection:
            _shadowDetection = newValue.toBool();
            return true;
        case pid::LearningRate:
            _learningRate = newValue.toDouble();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::History: return _history;
        case pid::VarianceThreshold: return _varThreshold;
        case pid::ShadowDetection: return _shadowDetection;
        case pid::LearningRate: return _learningRate;
        }

        return NodeProperty();
    }

    bool restart() override
    {
        _mog2 = cv::BackgroundSubtractorMOG2(_history, _varThreshold, _shadowDetection);
        return true;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& source = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImageMono();

        // Validate inputs
        if(source.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff - single step
        _mog2(source, output, _learningRate);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::Image, "input", "Input", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::ImageMono, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        static const PropertyConfig prop_config[] = {
            { EPropertyType::Integer, "History frames", "min:1, max:500" },
            { EPropertyType::Double, "Mahalanobis distance threshold",  "" },
            { EPropertyType::Boolean, "Detect shadow", "" },
            { EPropertyType::Double, "Learning rate", "min:-1, max:1, step:0.01, decimals:3" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Improved adaptive Gausian mixture model for background subtraction.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
        nodeConfig.flags = ENodeConfig::HasState;
    }

private:
    enum class pid
    {
        History,
        VarianceThreshold,
        ShadowDetection,
        LearningRate
    };

    double _learningRate;
    int _history;
    float _varThreshold;
    cv::BackgroundSubtractorMOG2 _mog2;
    bool _shadowDetection;
};

class BackgroundSubtractorGMGNodeType : public NodeType
{
public:
    BackgroundSubtractorGMGNodeType()
        : _learningRate(-1)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::LearningRate:
            _learningRate = newValue.toDouble();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::LearningRate: return _learningRate;
        }

        return NodeProperty();
    }

    bool restart() override
    {
        _gmg = cv::BackgroundSubtractorGMG();
        return true;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& source = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImageMono();

        // Validate inputs
        if(source.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff - single step
        _gmg(source, output, _learningRate);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::Image, "input", "Input", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::ImageMono, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        static const PropertyConfig prop_config[] = {
            { EPropertyType::Double, "Learning rate", "min:-1, max:1, step:0.01, decimals:3" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "GMG background subtractor.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
        nodeConfig.flags = ENodeConfig::HasState;
    }

private:
    enum class pid { LearningRate };

    double _learningRate;
    cv::BackgroundSubtractorGMG _gmg;
};

class BackgroundSubtractorNodeType : public NodeType
{
public:
    BackgroundSubtractorNodeType()
        : _alpha(0.92f)
        , _threshCoeff(3)
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Alpha:
            _alpha = newValue.toFloat();
            return true;
        case pid::ThresholdCoeff:
            _threshCoeff = newValue.toFloat();
            return true;
        }

        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Alpha: return _alpha;
        case pid::ThresholdCoeff: return _threshCoeff;
        }
        return NodeProperty();
    }

    bool restart() override
    {
        _frameN1 = cv::Mat();
        _frameN2 = cv::Mat();
        return true;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& frame = reader.readSocket(0).getImageMono();

        // Acquire output sockets
        cv::Mat& background = writer.acquireSocket(0).getImageMono();
        cv::Mat& movingPixels = writer.acquireSocket(1).getImageMono();
        cv::Mat& threshold = writer.acquireSocket(2).getImageMono();

        // Validate inputs
        if(frame.empty())
            return ExecutionStatus(EStatus::Ok);

        if(!_frameN2.empty()
            && _frameN2.size() == frame.size())
        {
            if(background.empty()
                || background.size() != frame.size())
            {
                background.create(frame.size(), CV_8UC1);
                background = cv::Scalar(0);
                movingPixels.create(frame.size(), CV_8UC1);
                movingPixels = cv::Scalar(0);
                threshold.create(frame.size(), CV_8UC1);
                threshold = cv::Scalar(127);
            }

            // Do stuff - single step
            cvu::parallel_for(cv::Range(0, frame.rows), [&](const cv::Range& range)
            {
                for(int y = range.start; y < range.end; ++y)
                {
                    for(int x = 0; x < frame.cols; ++x)
                    {
                        uchar thresh = threshold.at<uchar>(y, x);
                        uchar pix = frame.at<uchar>(y, x);

                        // Find moving pixels
                        bool moving = std::abs(pix - _frameN1.at<uchar>(y, x)) > thresh
                            && std::abs(pix - _frameN2.at<uchar>(y, x)) > thresh;
                        movingPixels.at<uchar>(y, x) = moving ? 255 : 0;

                        const int minThreshold = 20;

                        if(!moving)
                        {
                            // Update background image
                            uchar newBackground = cv::saturate_cast<uchar>(_alpha*float(background.at<uchar>(y, x)) + (1-_alpha)*float(pix));
                            background.at<uchar>(y, x) = newBackground;

                            // Update threshold image
                            float thresh = _alpha*float(threshold.at<uchar>(y, x)) + 
                                (1-_alpha)*(_threshCoeff * std::abs(pix - newBackground));
                            if(thresh > float(minThreshold))
                                threshold.at<uchar>(y, x) = cv::saturate_cast<uchar>(thresh);
                        }
                        else
                        {
                            // Update threshold image
                            threshold.at<uchar>(y, x) = minThreshold;
                        }
                    }
                }
            });
        }

        _frameN2 = _frameN1;
        _frameN1 = frame.clone();

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::ImageMono, "source", "Source", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::ImageMono, "background", "Background", "" },
            { ENodeFlowDataType::ImageMono, "movingPixels", "Moving pixels", "" },
            { ENodeFlowDataType::ImageMono, "threshold", "Threshold image", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Double, "Alpha", "min:0.0, max:1.0, decimals:3" },
            { EPropertyType::Double, "Threshold coeff.", "min:0.0, decimals:3" },
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
        nodeConfig.flags = ENodeConfig::HasState;
    }

private:
    enum class pid
    {
        Alpha,
        ThresholdCoeff
    };

    cv::Mat _frameN1; // I_{n-1}
    cv::Mat _frameN2; // I_{n-2}
    float _alpha;
    float _threshCoeff;
};

REGISTER_NODE("Video segmentation/Background subtractor", BackgroundSubtractorNodeType)
REGISTER_NODE("Video segmentation/GMG background subtractor", BackgroundSubtractorGMGNodeType)
REGISTER_NODE("Video segmentation/Adaptive mixture of Gaussians", AdaptiveMixtureOfGaussiansNodeType)
REGISTER_NODE("Video segmentation/Mixture of Gaussians", MixtureOfGaussiansNodeType)
