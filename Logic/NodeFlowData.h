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

#pragma once

#include "Prerequisites.h"

#ifndef Q_MOC_RUN
#include <boost/variant.hpp>
#endif

// OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>

#if defined(HAVE_OPENCL)
// Device data
#  include "OpenCL/DeviceArray.h"
#endif

using std::vector;

struct KeyPoints
{
    vector<cv::KeyPoint> kpoints;
    // could use shared_ptr BUT cv::Mat already
    // got reference counting under a table
    cv::Mat image;
};

struct Matches
{
    vector<cv::Point2f> queryPoints;
    vector<cv::Point2f> trainPoints;
    // same here
    cv::Mat queryImage;
    cv::Mat trainImage;
};

enum class ENodeFlowDataType : int
{
    Invalid,
    /// Image - 1 or 3 channels
    /// Node with such sockets supports both formats
    Image,
    /// Image with only 1 channel (monochromatic)
    ImageMono,
    /// Image with 3 channels (RGB)
    ImageRgb,
    Array,
    Keypoints,
    Matches
#if defined(HAVE_OPENCL)
    // Gpu part
    /// TODO: Could merge this with above ones and add bool flag
    ///       indicating if it's in device or host memory
    ,DeviceImage,
    DeviceImageMono,
    // In fact, its RGBA (BGRA, ARGB), as RGB noone seems to support
    DeviceImageRgb,
    DeviceArray,
#endif
};

namespace std
{
    LOGIC_EXPORT string to_string(ENodeFlowDataType);
}

class LOGIC_EXPORT NodeFlowData
{
    typedef boost::variant<
        cv::Mat,
        KeyPoints,
        Matches
    #if defined(HAVE_OPENCL)
        ,clw::Image2D, // TODO - replace this with some thin wrapper
        DeviceArray
    #endif
    > flow_data;

public:
    NodeFlowData();
    NodeFlowData(ENodeFlowDataType dataType);
    
    //NodeFlowData& operator=(const NodeFlowData& other);
    //NodeFlowData& operator=(NodeFlowData&& other);
    //NodeFlowData(const NodeFlowData& other);
    //NodeFlowData(NodeFlowData&& other);

    ~NodeFlowData();

    void saveToDisk(const std::string& filename) const;
    bool isValidImage() const;

    // Host data
    cv::Mat& getImage();
    const cv::Mat& getImage() const;

    cv::Mat& getImageMono();
    const cv::Mat& getImageMono() const;

    cv::Mat& getImageRgb();
    const cv::Mat& getImageRgb() const;

    KeyPoints& getKeypoints();
    const KeyPoints& getKeypoints() const;

    Matches& getMatches();
    const Matches& getMatches() const;

    cv::Mat& getArray();
    const cv::Mat& getArray() const;

    // Device data
#if defined(HAVE_OPENCL)
    clw::Image2D& getDeviceImage();
    const clw::Image2D& getDeviceImage() const;

    clw::Image2D& getDeviceImageMono();
    const clw::Image2D& getDeviceImageMono() const;

    clw::Image2D& getDeviceImageRgb();
    const clw::Image2D& getDeviceImageRgb() const;

    DeviceArray& getDeviceArray();
    const DeviceArray& getDeviceArray() const;
#endif

    bool isValid() const;
    bool canReturn(ENodeFlowDataType type) const;

    ENodeFlowDataType type() const;

private:
    ENodeFlowDataType _type;
    flow_data _data;

    bool isConvertible(ENodeFlowDataType from, 
        ENodeFlowDataType to) const;

    template <class Type> Type& getTyped(ENodeFlowDataType requestedType);
    template <class Type> const Type& getTyped(ENodeFlowDataType requestedType) const;
};

inline bool NodeFlowData::isValid() const
{ return _type != ENodeFlowDataType::Invalid; }

inline bool NodeFlowData::canReturn(ENodeFlowDataType type) const
{ return _type == type && isValid(); }

inline ENodeFlowDataType NodeFlowData::type() const
{ return _type; }
