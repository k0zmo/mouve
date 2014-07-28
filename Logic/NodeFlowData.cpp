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

#include "NodeFlowData.h"

#include <opencv2/highgui/highgui.hpp>

NodeFlowData::NodeFlowData()
    : _type(ENodeFlowDataType::Invalid)
    , _data()
{
}

NodeFlowData::NodeFlowData(ENodeFlowDataType dataType)
    : _type(dataType)
    , _data()
{
    switch(dataType)
    {
    case ENodeFlowDataType::Image:
    case ENodeFlowDataType::ImageMono:
    case ENodeFlowDataType::ImageRgb:
        _data = cv::Mat();
        break;
    case ENodeFlowDataType::Array:
        _data = cv::Mat();
        break;
    case ENodeFlowDataType::Keypoints:
        _data = KeyPoints();
        break;
    case ENodeFlowDataType::Matches:
        _data = Matches();
        break;
#if defined(HAVE_OPENCL)
    case ENodeFlowDataType::DeviceImage:
    case ENodeFlowDataType::DeviceImageMono:
    case ENodeFlowDataType::DeviceImageRgb:
        _data = clw::Image2D();
        break;
    case ENodeFlowDataType::DeviceArray:
        _data = DeviceArray();
        break;
#endif
    case ENodeFlowDataType::Invalid:
    default:
        break;
    }
}

NodeFlowData::~NodeFlowData()
{
}

void NodeFlowData::saveToDisk(const std::string& filename) const
{
    if(_type == ENodeFlowDataType::Image
        || _type == ENodeFlowDataType::ImageMono
        || _type == ENodeFlowDataType::ImageRgb)
    {
        const cv::Mat& img = boost::get<cv::Mat>(_data);
        if(!img.empty())
            cv::imwrite(filename, img);
    }
}

bool NodeFlowData::isValidImage() const
{
    if(_type == ENodeFlowDataType::Image
        || _type == ENodeFlowDataType::ImageMono
        || _type == ENodeFlowDataType::ImageRgb)
    {
        const cv::Mat& img = boost::get<cv::Mat>(_data);
        if(!img.empty())
            return true;
    }
    return false;
}

bool NodeFlowData::isConvertible(ENodeFlowDataType from, 
                                 ENodeFlowDataType to) const
{
    if(from == to)
        return true;

    switch(from)
    {
    case ENodeFlowDataType::Image:
        if(to == ENodeFlowDataType::ImageMono)
            return boost::get<cv::Mat>(_data).channels() == 1;
        else if(to == ENodeFlowDataType::ImageRgb)
            return boost::get<cv::Mat>(_data).channels() == 3;
        break;
    case ENodeFlowDataType::ImageMono:
        if(to == ENodeFlowDataType::Image)
            return boost::get<cv::Mat>(_data).channels() == 1;
        break;
    case ENodeFlowDataType::ImageRgb:
        if(to == ENodeFlowDataType::Image)
            return boost::get<cv::Mat>(_data).channels() == 3;
        break;
    case ENodeFlowDataType::Invalid:
    case ENodeFlowDataType::Array:
    case ENodeFlowDataType::Keypoints:
    case ENodeFlowDataType::Matches:
#if defined(HAVE_OPENCL)
    case ENodeFlowDataType::DeviceImage:
        if(to == ENodeFlowDataType::DeviceImageMono)
            return boost::get<clw::Image2D>(_data).bytesPerElement() == 1;
        else if(to == ENodeFlowDataType::DeviceImageRgb)
            return boost::get<clw::Image2D>(_data).bytesPerElement() == 4;
        break;
    case ENodeFlowDataType::DeviceImageMono:
        if(to == ENodeFlowDataType::DeviceImage)
            return boost::get<clw::Image2D>(_data).bytesPerElement() == 1;
        break;
    case ENodeFlowDataType::DeviceImageRgb:
        if(to == ENodeFlowDataType::DeviceImage)
            return boost::get<clw::Image2D>(_data).bytesPerElement() == 4;
        break;
    case ENodeFlowDataType::DeviceArray:
#endif
        break;
    }

    return false;
}

cv::Mat& NodeFlowData::getImage()
{ 
    return getType<cv::Mat>(ENodeFlowDataType::Image);
}

const cv::Mat& NodeFlowData::getImage() const 
{ 
    return getType<cv::Mat>(ENodeFlowDataType::Image);
}

cv::Mat& NodeFlowData::getImageMono()
{
    return getType<cv::Mat>(ENodeFlowDataType::ImageMono);
}

const cv::Mat& NodeFlowData::getImageMono() const
{
    return getType<cv::Mat>(ENodeFlowDataType::ImageMono);
}

cv::Mat& NodeFlowData::getImageRgb()
{
    return getType<cv::Mat>(ENodeFlowDataType::ImageRgb);
}

const cv::Mat& NodeFlowData::getImageRgb() const
{
    return getType<cv::Mat>(ENodeFlowDataType::ImageRgb);
}

KeyPoints& NodeFlowData::getKeypoints()
{
    return getType<KeyPoints>(ENodeFlowDataType::Keypoints);
}

const KeyPoints& NodeFlowData::getKeypoints() const
{
    return getType<KeyPoints>(ENodeFlowDataType::Keypoints);
}

Matches& NodeFlowData::getMatches()
{
    return getType<Matches>(ENodeFlowDataType::Matches);
}

const Matches& NodeFlowData::getMatches() const
{
    return getType<Matches>(ENodeFlowDataType::Matches);
}

cv::Mat& NodeFlowData::getArray()
{
    return getType<cv::Mat>(ENodeFlowDataType::Array);
}

const cv::Mat& NodeFlowData::getArray() const
{
    return getType<cv::Mat>(ENodeFlowDataType::Array);
}

#if defined(HAVE_OPENCL)

clw::Image2D& NodeFlowData::getDeviceImage()
{
    return getType<clw::Image2D>(ENodeFlowDataType::DeviceImage);
}

const clw::Image2D& NodeFlowData::getDeviceImage() const
{
    return getType<clw::Image2D>(ENodeFlowDataType::DeviceImage);
}

clw::Image2D& NodeFlowData::getDeviceImageMono()
{
    return getType<clw::Image2D>(ENodeFlowDataType::DeviceImageMono);
}

const clw::Image2D& NodeFlowData::getDeviceImageMono() const
{
    return getType<clw::Image2D>(ENodeFlowDataType::DeviceImageMono);
}

clw::Image2D& NodeFlowData::getDeviceImageRgb()
{
    return getType<clw::Image2D>(ENodeFlowDataType::DeviceImageRgb);
}

const clw::Image2D& NodeFlowData::getDeviceImageRgb() const
{
    return getType<clw::Image2D>(ENodeFlowDataType::DeviceImageRgb);
}

DeviceArray& NodeFlowData::getDeviceArray()
{
    return getType<DeviceArray>(ENodeFlowDataType::DeviceArray);
}

const DeviceArray& NodeFlowData::getDeviceArray() const
{
    return getType<DeviceArray>(ENodeFlowDataType::DeviceArray);
}

#endif

template <class Type>
Type& NodeFlowData::getType(ENodeFlowDataType requestedType)
{
    if(!isConvertible(_type, requestedType))
        throw boost::bad_get();
    return boost::get<Type>(_data);
}

template <class Type> 
const Type& NodeFlowData::getType(ENodeFlowDataType requestedType) const
{
    if(!isConvertible(_type, requestedType))
        throw boost::bad_get();
    return boost::get<Type>(_data);
}

namespace std
{
    string to_string(ENodeFlowDataType type)
    {
        switch(type)
        {
        default:
        case ENodeFlowDataType::Invalid:
            return "invalid";
        case ENodeFlowDataType::Image:
            return "image";
        case ENodeFlowDataType::ImageMono:
            return "imageMono";
        case ENodeFlowDataType::ImageRgb:
            return "imageRgb";
        case ENodeFlowDataType::Array:
            return "array";
        case ENodeFlowDataType::Keypoints:
            return "keypoints";
        case ENodeFlowDataType::Matches:
            return "matches";
#if defined(HAVE_OPENCL)
        case ENodeFlowDataType::DeviceImage:
            return "deviceImage";
        case ENodeFlowDataType::DeviceImageMono:
            return "deviceImageMono";
        case ENodeFlowDataType::DeviceImageRgb:
            return "deviceImageRgb";
        case ENodeFlowDataType::DeviceArray:
            return "deviceArray";
#endif
        }
    }
}