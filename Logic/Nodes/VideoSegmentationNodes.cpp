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
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::ImageMono);
        addProperty("History frames", _history)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 500))
            .setUiHints("min:1, max:500");
        addProperty("Number of mixtures", _nmixtures)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 9))
            .setUiHints("min:1, max:9");
        addProperty("Background ratio", _backgroundRatio)
            .setValidator(make_validator<ExclRangePropertyValidator<double>>(0.0, 1.0))
            .setUiHints("min:0.01, max:0.99, step:0.01");
        addProperty("Learning rate", _learningRate)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(-1.0, 1.0))
            .setUiHints("min:-1, max:1, step:0.01, decimals:3");
        setDescription("Gaussian Mixture-based image sequence background/foreground segmentation.");
        setFlags(ENodeConfig::HasState);
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

private:
    cv::BackgroundSubtractorMOG _mog;
    TypedNodeProperty<int> _history;
    TypedNodeProperty<int> _nmixtures;
    TypedNodeProperty<double> _backgroundRatio;
    TypedNodeProperty<double> _learningRate;
};

class AdaptiveMixtureOfGaussiansNodeType : public NodeType
{
public:
    AdaptiveMixtureOfGaussiansNodeType()
        : _history(200)
        , _varThreshold(16.0f)
        , _shadowDetection(true)
        , _learningRate(-1)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::ImageMono);
        addProperty("History frames", _history)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 500))
            .setUiHints("min:1, max:500");
        addProperty("Mahalanobis distance threshold", _varThreshold);
        addProperty("Detect shadow", _shadowDetection);
        addProperty("Learning rate", _learningRate)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(-1.0, 1.0))
            .setUiHints("min:-1, max:1, step:0.01, decimals:3");
        setDescription("Improved adaptive Gausian mixture model for background subtraction.");
        setFlags(ENodeConfig::HasState);
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

private:
    enum class pid
    {
        History,
        VarianceThreshold,
        ShadowDetection,
        LearningRate
    };

    TypedNodeProperty<int> _history;
    TypedNodeProperty<float> _varThreshold;
    TypedNodeProperty<bool> _shadowDetection;
    TypedNodeProperty<double> _learningRate;
    cv::BackgroundSubtractorMOG2 _mog2;
};

class BackgroundSubtractorGMGNodeType : public NodeType
{
public:
    BackgroundSubtractorGMGNodeType()
        : _learningRate(-1)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::ImageMono);
        addProperty("Learning rate", _learningRate)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(-1, 1))
            .setUiHints("min:-1, max:1, step:0.01, decimals:3");
        setDescription("GMG background subtractor.");
        setFlags(ENodeConfig::HasState);
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


private:
    TypedNodeProperty<double> _learningRate;
    cv::BackgroundSubtractorGMG _gmg;
};

class BackgroundSubtractorNodeType : public NodeType
{
public:
    BackgroundSubtractorNodeType()
        : _alpha(0.92f)
        , _threshCoeff(3)
    {
        addInput("Source", ENodeFlowDataType::ImageMono);
        addOutput("Background", ENodeFlowDataType::ImageMono);
        addOutput("Moving pixels", ENodeFlowDataType::ImageMono);
        addOutput("Threshold image", ENodeFlowDataType::ImageMono);
        addProperty("Alpha", _alpha)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 1.0))
            .setUiHints("min:0.0, max:1.0, decimals:3");
        addProperty("Threshold coeff.", _threshCoeff)
            .setValidator(make_validator<MinPropertyValidator<double>>(0.0))
            .setUiHints("min:0.0, decimals:3");
        setFlags(ENodeConfig::HasState);
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

private:
    cv::Mat _frameN1; // I_{n-1}
    cv::Mat _frameN2; // I_{n-2}
    TypedNodeProperty<float> _alpha;
    TypedNodeProperty<float> _threshCoeff;
};

REGISTER_NODE("Video segmentation/Background subtractor", BackgroundSubtractorNodeType)
REGISTER_NODE("Video segmentation/GMG background subtractor", BackgroundSubtractorGMGNodeType)
REGISTER_NODE("Video segmentation/Adaptive mixture of Gaussians", AdaptiveMixtureOfGaussiansNodeType)
REGISTER_NODE("Video segmentation/Mixture of Gaussians", MixtureOfGaussiansNodeType)
