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

#include "IJaiNodeModule.h"

#if defined(HAVE_JAI)

#include "Logic/NodeModule.h"

#include <Jai_Factory.h>

using std::string;
using std::vector;
using std::tuple;

/// TODO:
/// * Support for 10-bit bayer formats
/// * White-balance RGB gain finder
/// * (LONG) Use lower primitives (StreamThread instead of StreamCallback)

class MOUVE_EXPORT JaiNodeModule : public IJaiNodeModule
{
public:
    JaiNodeModule();
    ~JaiNodeModule() override;

    bool initialize() override;
    bool isInitialized() override;

    string moduleName() const override;

    int cameraCount() const override { return int(_camHandles.size()); }
    vector<CameraInfo> discoverCameras(EDriverType driverType = EDriverType::Filter) override;
    CameraSettings cameraSettings(int index) override;
    bool setCameraSettings(int index, const CameraSettings& settings)  override;

    CAM_HANDLE openCamera(int index);
    void closeCamera(CAM_HANDLE camHandle);

private:
    FACTORY_HANDLE _factoryHandle;
    vector<CAM_HANDLE> _camHandles;
};

#define NODE_NAME_OFFSETX "OffsetX"
#define NODE_NAME_OFFSETY "OffsetY"
#define NODE_NAME_WIDTH "Width"
#define NODE_NAME_HEIGHT "Height"
#define NODE_NAME_PIXELFORMAT "PixelFormat"
#define NODE_NAME_GAIN "GainRaw"
#define NODE_NAME_ACQSTART "AcquisitionStart"
#define NODE_NAME_ACQSTOP "AcquisitionStop"

template<typename T>
inline T queryNodeValue(CAM_HANDLE hCamera, const char* nodeName)
{ return T(); }

template<> int64_t queryNodeValue<int64_t>(CAM_HANDLE hCamera, const char* nodeName);

#endif
