/*
 * Copyright (c) 2013-2018 Kajetan Swierk <k0zmo@outlook.com>
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

#include <opencv2/core/core.hpp>
#include <opencv2/core/core_c.h>

enum class EColor
{
    AllRandom,
    Red,
    Green,
    Blue
};

inline cv::Scalar getColor(EColor color)
{
    switch (color)
    {
    case EColor::Red:
        return cv::Scalar{36, 28, 237};
    case EColor::Green:
        return cv::Scalar{76, 177, 34};
    case EColor::Blue:
        return cv::Scalar{244, 63, 72};
    }
    return cv::Scalar::all(-1);
}

inline cv::Scalar getRandomColor(cv::RNG& rng)
{
    return cv::Scalar(rng(256), rng(256), rng(256));
}

enum class ELineType
{
    Line_4Connected = 4,
    Line_8Connected = 8,
    Line_AA = CV_AA
};
