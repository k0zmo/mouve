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
    {
    }

    bool setProperty(PropertyID propId, const NodeProperty& newValue) override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Filename:
            _filename = newValue.toFilepath();
            return true;
        case pid::Fps:
            _fps = newValue.toDouble();
            return true;
        case pid::FourCC:
            _fourcc = newValue.toEnum().cast<FOURCC>();
            return true;
        }
        return false;
    }

    NodeProperty property(PropertyID propId) const override
    {
        switch(static_cast<pid>(propId))
        {
        case pid::Filename: return _filename;
        case pid::Fps: return _fps;
        case pid::FourCC: return _fourcc;
        }

        return NodeProperty();
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
            _writer.open(_filename.data(), getFOURCC(_fourcc), _fps, input.size(), isColor);
            if(!_writer.isOpened())
                return ExecutionStatus(EStatus::Error, 
                    string_format("Couldn't open video writer for file %s", _filename.data().c_str()));
        }

        _writer << input;

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::Image, "input", "Input", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const PropertyConfig prop_config[] = {
            { EPropertyType::Filepath, "Filepath", "filter: Video files (*.avi), save: true" },
            { EPropertyType::Double, "FPS", "min: 1.0" },
            { EPropertyType::Enum, "FOURCC", "item: IYUV, item: MJPG" },			
            { EPropertyType::Unknown, "", "" }
        };

        nodeConfig.description = "Saves incoming images as a video";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
        nodeConfig.pProperties = prop_config;
        nodeConfig.flags = ENodeConfig::HasState;
    }

private:
    enum class pid
    {
        Filename,
        Fps,
        FourCC
    };

    cv::VideoWriter _writer;
    double _fps;
    FOURCC _fourcc;
    Filepath _filename;
};

REGISTER_NODE("Sink/Video writer", VideoWriterNodeType)