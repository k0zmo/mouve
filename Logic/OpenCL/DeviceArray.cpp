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

#if defined(HAVE_OPENCL)

#include "DeviceArray.h"

DeviceArray::DeviceArray()
    : _width(0)
    , _height(0)
    , _size(0)
{
}

DeviceArray DeviceArray::create(clw::Context& context,
                                clw::EAccess access, 
                                clw::EMemoryLocation location,
                                int width, 
                                int height, 
                                EDataType dataType)
{
    size_t size = width * height * dataSize(dataType);
    clw::Buffer buffer = context.createBuffer(access, location, size);
    DeviceArray deviceArray;
    if(!buffer.isNull())
    {
        deviceArray._buffer = std::move(buffer);
        deviceArray._width = width;
        deviceArray._height = height;
        deviceArray._dataType = dataType;
        deviceArray._size = size;
    }

    return deviceArray;
}

DeviceArray DeviceArray::createFromBuffer(clw::Buffer& buffer,
                                          int width, 
                                          int height, 
                                          EDataType dataType)
{
    DeviceArray deviceArray;

    if(!buffer.isNull())
    {
        deviceArray._buffer = buffer;
        deviceArray._width = width;
        deviceArray._height = height;
        deviceArray._dataType = dataType;
        deviceArray._size = width * height * dataSize(dataType);;
    }

    return deviceArray;
}

void DeviceArray::truncate(int height)
{
    if(_height > height)
    {
        _height = height;
        _size = _width * _height * dataSize(_dataType);
    }
}

clw::Event DeviceArray::upload(clw::CommandQueue& queue, const void* data)
{
    /// TODO Benchmark direct access with mapped one
    /// TODO Check pinned memory
    if(_buffer.isNull())
        return clw::Event();
    return queue.asyncWriteBuffer(_buffer, data, 0, _size);
}

clw::Event DeviceArray::download(clw::CommandQueue& queue, void* data) const
{
    if(_buffer.isNull())
        return clw::Event();
    return queue.asyncReadBuffer(_buffer, data, 0, _size);
}

clw::Event DeviceArray::download(clw::CommandQueue& queue, cv::Mat& mat) const
{
    if(_buffer.isNull())
        return clw::Event();

    mat.create(_height, _width, deviceToMatType(_dataType));
    return queue.asyncReadBuffer(_buffer, mat.data, 0, _size);
}

#endif