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

#if defined(HAVE_JAI)

#include "NodeType.h"
#include "NodeFactory.h"

#include "JaiNodeModule.h"
#include "JaiException.h"

#include <opencv2/core/core.hpp>
#include <mutex>

class JaiCameraNodeType : public NodeType
{
public:
    JaiCameraNodeType()
        : _hCamera(nullptr)
        , _hStreamThread(nullptr)
        , _width(0)
        , _height(0)
        , _lastTimeStamp(0)
    {
        addOutput("Output", ENodeFlowDataType::Image);
        setDescription("Provides video frames from JAI camera");
        setModule("jai");
        setFlags(ENodeConfig::HasState | ENodeConfig::AutoTag);
    }

    bool init(const std::shared_ptr<NodeModule>& nodeModule) override
    {
        try
        {
            _jaiModule = std::dynamic_pointer_cast<JaiNodeModule>(nodeModule);
            if(!_jaiModule)
                return false;

            _hCamera = _jaiModule->openCamera(0);
            return _hCamera != nullptr;
        }
        catch(JaiException&)
        {
            return false;
        }
    }	
    
    bool restart() override
    {
        _width = int(queryNodeValue<int64_t>(_hCamera, NODE_NAME_WIDTH));
        _height = int(queryNodeValue<int64_t>(_hCamera, NODE_NAME_HEIGHT));
        int64_t pixelFormat = queryNodeValue<int64_t>(_hCamera, NODE_NAME_PIXELFORMAT);
        int bpp = J_BitsPerPixel(pixelFormat);
        _lastTimeStamp = 0;

        switch(bpp)
        {
        case 8:
            _sourceFrame.create(_height, _width, CV_8UC1);
            break;
        case 16:
            _sourceFrame.create(_height, _width, CV_16UC1);
            break;
        }
        
        // Open stream
        J_STATUS_TYPE error;
        if ((error = J_Image_OpenStream(_hCamera, 0, reinterpret_cast<J_IMG_CALLBACK_OBJECT>(this),
            reinterpret_cast<J_IMG_CALLBACK_FUNCTION>(&JaiCameraNodeType::cameraStreamCallback),
            &_hStreamThread, (_width * _height * bpp)/8)) != J_ST_SUCCESS)
        {
            throw JaiException(error, "Stream couldn't be opened");
        }

        // Start Acquision
        if ((error = J_Camera_ExecuteCommand(_hCamera, 
            (int8_t*)NODE_NAME_ACQSTART)) != J_ST_SUCCESS)
        {
            throw JaiException(error, "Acquisition couldn't be started");
        }
        
        return true;
    }
    
    void cameraStreamCallback(J_tIMAGE_INFO* pAqImageInfo)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        int64_t diff = pAqImageInfo->iTimeStamp - _lastTimeStamp;
        if(diff < 0)
        {
            // udp packet disordered, ignore it
        }
        else
        {
            _lastTimeStamp = pAqImageInfo->iTimeStamp;

            if(_sourceFrame.step * _sourceFrame.rows == pAqImageInfo->iImageSize)
                memcpy(_sourceFrame.data, pAqImageInfo->pImageBuffer, pAqImageInfo->iImageSize);
        }
    }
    
    ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
    {
        cv::Mat& matFrame = writer.acquireSocket(0).getImage();
        {
            std::lock_guard<std::mutex> lock(_mutex);
            matFrame = _sourceFrame;
        }
        return ExecutionStatus(EStatus::Tag);
    }
    
    void finish() override
    {
        J_STATUS_TYPE error;
        
        // Stop Acquision
        if (_hCamera)
        {
            if ((error = J_Camera_ExecuteCommand(_hCamera,
                (int8_t*)NODE_NAME_ACQSTOP)) != J_ST_SUCCESS)
            {}// Doesn't make any sense to do anything here
        }

        if(_hStreamThread)
        {
            // Close stream
            if ((error = J_Image_CloseStream(_hStreamThread)) != J_ST_SUCCESS)
            {}// Doesn't make any sense to do anything here
            _hStreamThread = nullptr;
        }
    }
    
private:
    CAM_HANDLE _hCamera;
    THRD_HANDLE _hStreamThread;
    int _width;
    int _height;
    cv::Mat _sourceFrame;
    std::mutex _mutex;
    uint64_t _lastTimeStamp;

    std::shared_ptr<JaiNodeModule> _jaiModule;
};

REGISTER_NODE("Source/JAI Camera", JaiCameraNodeType)

#endif