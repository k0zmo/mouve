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

#include "Logic/NodeModule.h"

template<typename T>
struct RangedValue
{
    RangedValue() : minValue(0), maxValue(0), value(0) {}
    RangedValue(T minValue, T maxValue, T value)
        : minValue(minValue), maxValue(maxValue), value(value) {}

    T minValue;
    T maxValue;
    T value;
};

struct CameraSettings
{
    RangedValue<int64_t> offsetX;
    RangedValue<int64_t> offsetY;
    RangedValue<int64_t> width;
    RangedValue<int64_t> height;
    RangedValue<int64_t> gain;
    int64_t pixelFormat;
    std::vector<std::tuple<int64_t, std::string>> pixelFormats;	
};

struct CameraInfo
{
    std::string id;
    std::string modelName;
    std::string manufacturer;
    std::string interfaceId;
    std::string ipAddress;
    std::string macAddress;
    std::string serialNumber;
    std::string userName;
};

enum class EDriverType { All, Filter, Socket };

class LOGIC_EXPORT IJaiNodeModule : public NodeModule
{
public:
    virtual int cameraCount() const = 0;
    virtual std::vector<CameraInfo> discoverCameras(EDriverType driverType = EDriverType::Filter) = 0;
    virtual CameraSettings cameraSettings(int index) = 0;
    virtual bool setCameraSettings(int index, const CameraSettings& settings) = 0;
};

// Will return null if Logic wasn't compiled with support of JAI cameras
LOGIC_EXPORT std::unique_ptr<IJaiNodeModule> createJaiModule();