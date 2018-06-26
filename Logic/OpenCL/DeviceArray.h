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

#if defined(HAVE_OPENCL)

#include "../Prerequisites.h"

#include <clw/clw.h>
#include <opencv2/core/core.hpp>

enum class EDataType
{
    Uchar,
    Char,
    Ushort,
    Short,
    Int,
    Float,
    Double
};

class MOUVE_EXPORT DeviceArray
{
public:
    DeviceArray();

    bool isNull() const { return _buffer.isNull(); }

    static DeviceArray create(clw::Context& context, 
        clw::EAccess access, clw::EMemoryLocation location,
        int width, int height, EDataType dataType);

    static DeviceArray createFromBuffer(clw::Buffer& buffer, 
        int width, int height, EDataType dataType);

    void truncate(int height);
    //void resize(int width, int height);

    int width() const { return _width; }
    int height() const { return _height; }
    size_t size() const { return _size; }
    EDataType dataType() const { return _dataType; }
    const clw::Buffer& buffer() const { return _buffer; }
    clw::Buffer& buffer() { return _buffer; }

    clw::Event upload(clw::CommandQueue& queue, const void* data);
    clw::Event download(clw::CommandQueue& queue, void* data) const;
    clw::Event download(clw::CommandQueue& queue, cv::Mat& mat) const;

    static int deviceToMatType(EDataType dataType);
    static EDataType matToDeviceType(int type);

    static size_t dataSize(EDataType dataType);

private:
    int _width;
    int _height;
    EDataType _dataType;
    size_t _size;
    clw::Buffer _buffer;
};

inline size_t DeviceArray::dataSize(EDataType dataType)
{
    switch(dataType)
    {
    case EDataType::Uchar: return sizeof(unsigned char);
    case EDataType::Char: return sizeof(char);
    case EDataType::Ushort: return sizeof(unsigned short);
    case EDataType::Short: return sizeof(short);
    case EDataType::Int: return sizeof(int);
    case EDataType::Float: return sizeof(float);
    case EDataType::Double: return sizeof(double);
    default: return 0;
    }
}

inline int DeviceArray::deviceToMatType(EDataType dataType)
{
    switch(dataType)
    {
    case EDataType::Uchar: return CV_8U;
    case EDataType::Char: return CV_8S;
    case EDataType::Ushort: return CV_16U;
    case EDataType::Short: return CV_16S;
    case EDataType::Int: return CV_32S;
    case EDataType::Float: return CV_32F;
    case EDataType::Double: return CV_64F;
    // just to shut up compiler
    default: return 0;
    }
}

inline EDataType DeviceArray::matToDeviceType(int type)
{
    switch(type)
    {
    case CV_8U: return EDataType::Uchar;
    case CV_8S: return EDataType::Char;
    case CV_16U: return EDataType::Ushort;
    case CV_16S: return EDataType::Short;
    case CV_32S: return EDataType::Int;
    case CV_32F: return EDataType::Float;
    case CV_64F: return EDataType::Double;
    // just to shut up compiler
    default: return EDataType::Char; 
    }
}

#endif
