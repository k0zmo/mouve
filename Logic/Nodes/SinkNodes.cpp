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

#include <fmt/core.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

enum class FOURCC
{
    IYUV,
    MJPG
};

int getFOURCC(FOURCC fcc)
{
    switch(fcc)
    {
    case FOURCC::IYUV: return CV_FOURCC('I','Y','U','V');
    case FOURCC::MJPG: return CV_FOURCC('M','J','P','G');
    }
    return -1;
}

class VideoWriterNodeType : public NodeType
{
public:
    VideoWriterNodeType()
        : _fps(25.0)
        , _fourcc(FOURCC::MJPG)
        , _filename(Filepath{})
    {
        addInput("Input", ENodeFlowDataType::Image);
        addProperty("Filepath", _filename)
            .setUiHints("filter: Video files (*.avi), save: true");
        addProperty("FPS", _fps)
            .setValidator(make_validator<MinPropertyValidator<double>>(1.0))
            .setUiHints("min: 1.0");
        addProperty("FOURCC", _fourcc)
            .setUiHints("item: IYUV, item: MJPG");
        setDescription("Saves incoming images as a video");
        setFlags(kl::make_flags(ENodeConfig::HasState));
    }

    bool restart() override
    {
        return true;
    }

    void finish() override
    {
        if(_writer.isOpened())
            _writer.release();
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter&) override
    {
        // Read input sockets
        const cv::Mat& input = reader.readSocket(0).getImage();

        // Validate inputs
        if(input.empty())
            return ExecutionStatus(EStatus::Ok);

        if(!_writer.isOpened())
        {
            bool isColor = input.channels() == 3;
            _writer.open(_filename.cast<Filepath>().data(), getFOURCC(_fourcc), _fps, input.size(), isColor);
            if (!_writer.isOpened())
            {
                return ExecutionStatus(EStatus::Error,
                                       fmt::format("Couldn't open video writer for file {}",
                                                   _filename.cast<Filepath>().data()));
            }
        }

        _writer << input;

        return ExecutionStatus(EStatus::Ok);
    }

private:
    cv::VideoWriter _writer;
    TypedNodeProperty<double> _fps;
    TypedNodeProperty<FOURCC> _fourcc;
    TypedNodeProperty<Filepath> _filename;
};

REGISTER_NODE("Sink/Video writer", VideoWriterNodeType)