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

#include "CV.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace cvu {

int bayerCodeGray(EBayerCode code)
{
    switch(code)
    {
    // OpenCV code is shifted one down and one left from
    // the one that GigE Vision cameras are using
    case EBayerCode::BG: return CV_BayerRG2GRAY;
    case EBayerCode::GB: return CV_BayerGR2GRAY;
    case EBayerCode::RG: return CV_BayerBG2GRAY;
    case EBayerCode::GR: return CV_BayerGB2GRAY;
    default: return CV_BayerBG2GRAY;
    }
}

int bayerCodeRgb(EBayerCode code)
{
    switch(code)
    {
    case EBayerCode::BG: return CV_BayerRG2BGR;
    case EBayerCode::GB: return CV_BayerGR2BGR;
    case EBayerCode::RG: return CV_BayerBG2BGR;
    case EBayerCode::GR: return CV_BayerGB2BGR;
    default: return CV_BayerBG2GRAY;
    }
}
}
