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
#include "Kommon/HighResolutionClock.h"
#include "Kommon/StringUtils.h"

#include <chrono>
#include <thread>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

class VideoFromFileNodeType : public NodeType
{
public:
    VideoFromFileNodeType()
#ifdef QT_DEBUG
        : _videoPath("video-4.mkv")
#else
        : _videoPath(Filepath(""))
#endif
        , _startFrame(0U)
        , _endFrame(0U)
        , _frameInterval(0U)
        , _ignoreFps(false)
        , _forceGrayscale(false)
    {
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Video path", _videoPath)
            .setUiHints("filter:Video files (*.mkv *.mp4 *.avi *.flv)");
        addProperty("Start frame", _startFrame)
            .setValidator(make_validator<MinPropertyValidator<int>>(0))
            .setUiHints("min:0");
        addProperty("End frame", _endFrame)
            .setValidator(make_validator<MinPropertyValidator<int>>(0))
            .setUiHints("min:0");
        addProperty("Ignore FPS", _ignoreFps);
        addProperty("Force grayscale", _forceGrayscale);
        setDescription("Provides video frames from specified stream.");
        setFlags(ENodeConfig::HasState | ENodeConfig::AutoTag | ENodeConfig::OverridesTimeComputation);
    }

    bool restart() override
    {
        // This isn't really necessary as subsequent open()
        // should call it automatically
        if(_capture.isOpened())
            _capture.release();

        _capture.open(_videoPath.cast<Filepath>().data());
        if(!_capture.isOpened())
            return false;

        if(_startFrame > 0)
            _capture.set(CV_CAP_PROP_POS_FRAMES, _startFrame);

        double fps = _ignoreFps ? 0 : _capture.get(CV_CAP_PROP_FPS);

        _frameInterval = (fps != 0)
            ? int(ceil(1000.0 / fps))
            : 0;
        _timeStamp = HighResolutionClock::time_point{};
        _maxFrames = static_cast<int>(_capture.get(CV_CAP_PROP_FRAME_COUNT));
        _currentFrame = _startFrame > 0 ? _startFrame : 0;

        return true;
    }

    ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
    {
        HighResolutionClock::time_point start = HighResolutionClock::now();

        if(!_capture.isOpened())
            return ExecutionStatus(EStatus::Ok);

        cv::Mat& output = writer.acquireSocket(0).getImage();

        ++_currentFrame;

        if(_endFrame > 0 && _currentFrame >= _endFrame)
        {
            // No more data (for us)
            return ExecutionStatus(EStatus::Ok,
                string_format("Frame image width: %d\nFrame image height: %d\nFrame channels count: %d\nFrame size in kbytes: %d\nFrame: %d/%d",
                    output.cols, 
                    output.rows,
                    output.channels(),
                    output.cols * output.rows * sizeof(uchar) * output.channels() / 1024, 
                    _currentFrame - 1,
                    _maxFrames));
        }

        // We need this so we won't replace previous good frame with current bad (for instance - end of video)
        cv::Mat buffer; 

        if(_frameInterval == 0)
        {
            _capture.read(buffer);
        }
        else
        {
            using namespace std::chrono;

            // This should keep FPS in sync
            HighResolutionClock::time_point s = HighResolutionClock::now();
            milliseconds dura = duration_cast<milliseconds>(s - _timeStamp);
            if(dura.count() < _frameInterval)
            {
                milliseconds waitDuration = milliseconds(_frameInterval) - dura;
                std::this_thread::sleep_for(waitDuration);
                // New start time
                start = HighResolutionClock::now();
            }

            _capture.read(buffer);

            _timeStamp = HighResolutionClock::now();
        }

        if(!buffer.empty())
        {
            _forceGrayscale.cast_value<bool>();
            if(_forceGrayscale && buffer.channels() > 1)
                cv::cvtColor(buffer, output, CV_BGR2GRAY);
            else
                output = buffer;

            HighResolutionClock::time_point stop = HighResolutionClock::now();
            return ExecutionStatus(EStatus::Tag, convertToMilliseconds(stop - start),
                string_format("Frame image width: %d\nFrame image height: %d\nFrame channels count: %d\nFrame size in kbytes: %d\nFrame: %d/%d",
                    output.cols,
                    output.rows,
                    output.channels(),
                    output.cols * output.rows * sizeof(uchar) * output.channels() / 1024,
                    _currentFrame,
                    _maxFrames));
        }
        else
        {
            // No more data 
            return ExecutionStatus(EStatus::Ok);
        }
    }

private:
    TypedNodeProperty<Filepath> _videoPath;
    cv::VideoCapture _capture;
    TypedNodeProperty<int> _startFrame;
    TypedNodeProperty<int> _endFrame;
    int _frameInterval;
    int _currentFrame;
    int _maxFrames;
    HighResolutionClock::time_point _timeStamp;
    TypedNodeProperty<bool> _ignoreFps;
    TypedNodeProperty<bool> _forceGrayscale;
};

class ImageFromFileNodeType : public NodeType
{
public: 
    ImageFromFileNodeType()
#ifdef QT_DEBUG
        : _filePath("lena.jpg")
#else
        : _filePath(Filepath(""))
#endif
        , _forceGrayscale(false)
    {
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("File path", _filePath)
            .setUiHints("filter:"
                "Popular image formats (*.bmp *.jpeg *.jpg *.png *.tiff);;"
                "Windows bitmaps (*.bmp *.dib);;"
                "JPEG files (*.jpeg *.jpg *.jpe);;"
                "JPEG 2000 files (*.jp2);;"
                "Portable Network Graphics (*.png);;"
                "Portable image format (*.pbm *.pgm *.ppm);;"
                "Sun rasters (*.sr *.ras);;"
                "TIFF files (*.tiff *.tif);;"
                "All files (*.*)");
        addProperty("Force grayscale", _forceGrayscale);
        setDescription("Loads image from a given location.");
    }

    ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
    {
        cv::Mat& output = writer.acquireSocket(0).getImage();

        output = cv::imread(_filePath.cast<Filepath>().data(), _forceGrayscale ? CV_LOAD_IMAGE_GRAYSCALE : CV_LOAD_IMAGE_UNCHANGED);
        if(output.channels() == 4)
            cv::cvtColor(output, output, CV_BGRA2BGR);

        if(output.empty())
            return ExecutionStatus(EStatus::Error, "File not found");
        return ExecutionStatus(EStatus::Ok, 
            string_format("Image image width: %d\nImage image height: %d\nImage channels count: %d\nImage size in kbytes: %d",
                output.cols, 
                output.rows,
                output.channels(),
                output.cols * output.rows * sizeof(uchar) * output.channels() / 1024));
    }

protected:

    TypedNodeProperty<Filepath> _filePath;
    TypedNodeProperty<bool> _forceGrayscale;
};

class ImageFromFileStreamNodeType : public ImageFromFileNodeType
{
public: 
    ImageFromFileStreamNodeType()
        : ImageFromFileNodeType()
    {
        setFlags(ENodeConfig::AutoTag | ENodeConfig::HasState);
    }

    bool restart() override
    { 
        _img = cv::imread(_filePath.cast<Filepath>().data(), _forceGrayscale ? CV_LOAD_IMAGE_GRAYSCALE : CV_LOAD_IMAGE_UNCHANGED);

        return !_img.empty();
    }

    ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
    {
        cv::Mat& output = writer.acquireSocket(0).getImage();

        output = _img;

        return ExecutionStatus(EStatus::Tag, 
            string_format("Image image width: %d\nImage image height: %d\nImage channels count: %d\nImage size in kbytes: %d",
                output.cols, 
                output.rows,
                output.channels(),
                output.cols * output.rows * sizeof(uchar) * output.channels() / 1024));
    }

private:
    cv::Mat _img;
};

class CameraCaptureNodeType : public NodeType
{
public:
    CameraCaptureNodeType()
        : _deviceId(0)
        , _forceGrayscale(false)
    {
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Device Id", _deviceId)
            .setUiHints("min:0")
            .setValidator(make_validator<MinPropertyValidator<int>>(0));
        setDescription("Provides video frames from specified camera device.");
        setFlags(ENodeConfig::HasState | ENodeConfig::AutoTag);
    }

    bool restart() override
    {
        _capture.open(_deviceId);
        if(!_capture.isOpened())
            return false;
        return true;
    }

    ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
    {
        cv::Mat& output = writer.acquireSocket(0).getImage();

        /// TODO: Could be moved to background thread (just like JAI)
        cv::Mat tmp;
        _capture >> tmp;

        if(tmp.empty())
            return ExecutionStatus(EStatus::Ok);

        if(_forceGrayscale && tmp.channels() > 1)
            cv::cvtColor(tmp, output, CV_BGR2GRAY);
        else
            output = tmp;

        return ExecutionStatus(EStatus::Tag, 
            string_format("Frame image width: %d\nFrame image height: %d\nFrame channels count: %d\nFrame size in kbytes: %d",
                output.cols,
                output.rows, 
                output.channels(),
                output.cols * output.rows * sizeof(uchar) * output.channels() / 1024));
    }

    void finish() override
    {
        if(_capture.isOpened())
            _capture.release();
    }

private:
    TypedNodeProperty<int> _deviceId;
    TypedNodeProperty<bool> _forceGrayscale;
    cv::VideoCapture _capture;
};

class SolidImageNodeType : public NodeType
{
public: 
    SolidImageNodeType()
        : _width(512)
        , _height(512)
        , _gray(0)
    {
        addOutput("Output", ENodeFlowDataType::ImageMono);
        addProperty("Width", _width)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 4096))
            .setUiHints("min:1, max:4096");
        addProperty("Height", _height)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 4096))
            .setUiHints("min:1, max:4096");
        addProperty("Gray", _gray)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(0, 255))
            .setUiHints("min:0, max:255");
        setDescription("Creates solid one-color (grayscale) image");
    }

    ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
    {
        cv::Mat& output = writer.acquireSocket(0).getImageMono();
        if(_width * _height > 0)
            output = cv::Mat(_height, _width, CV_8UC1, cv::Scalar(_gray));
        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<int> _width;
    TypedNodeProperty<int> _height;
    TypedNodeProperty<int> _gray;
};

class SolidRgbImageNodeType : public NodeType
{
public: 
    SolidRgbImageNodeType()
        : _width(512)
        , _height(512)
        , _red(0)
        , _green(0)
        , _blue(0)
    {
        addOutput("Output", ENodeFlowDataType::ImageRgb);
        addProperty("Width", _width)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 4096))
            .setUiHints("min:1, max:4096");
        addProperty("Height", _height)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 4096))
            .setUiHints("min:1, max:4096");
        addProperty("Red", _red)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(0, 255))
            .setUiHints("min:0, max:255");
        addProperty("Green", _green)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(0, 255))
            .setUiHints("min:0, max:255");
        addProperty("Blue", _blue)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(0, 255))
            .setUiHints("min:0, max:255");
        setDescription("Creates solid RGB image");
    }

    ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
    {
        cv::Mat& output = writer.acquireSocket(0).getImageRgb();
        if(_width * _height > 0)
            output = cv::Mat(_height, _width, CV_8UC3, cv::Scalar(_blue, _green, _red));
        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<int> _width;
    TypedNodeProperty<int> _height;
    TypedNodeProperty<int> _red;
    TypedNodeProperty<int> _green;
    TypedNodeProperty<int> _blue;
};

REGISTER_NODE("Sources/Solid image", SolidRgbImageNodeType)
REGISTER_NODE("Sources/Solid gray image", SolidImageNodeType)
REGISTER_NODE("Sources/Image from file stream", ImageFromFileStreamNodeType)
REGISTER_NODE("Sources/Camera capture", CameraCaptureNodeType)
REGISTER_NODE("Sources/Video from file", VideoFromFileNodeType)
REGISTER_NODE("Sources/Image from file", ImageFromFileNodeType)
