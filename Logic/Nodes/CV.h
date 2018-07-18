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

#include "../Prerequisites.h"

#include <opencv2/core/core.hpp>

namespace cvu  {

enum class EBayerCode
{
    BG,
    GB,
    RG,
    GR
};

int bayerCodeGray(EBayerCode code);
int bayerCodeRgb(EBayerCode code);

// Temporary here
enum class EColor
{
    Random,
    Red,
    Green,
    Blue,
    Yellow
};

inline EColor getPairedColor(EColor color)
{
    switch (color)
    {
    case EColor::Random:
        return EColor::Random;
    case EColor::Red:
        return EColor::Green;
    case EColor::Green:
        return EColor::Red;
    case EColor::Blue:
        return EColor::Yellow;
    case EColor::Yellow:
        return EColor::Blue;
    }

    return color;
}

template <typename T = cv::Scalar>
T getColor(EColor color)
{
    switch (color)
    {
    case EColor::Red:
        return {36, 28, 237};
    case EColor::Green:
        return {76, 177, 34};
    case EColor::Blue:
        return {244, 63, 72};
    case EColor::Yellow:
        return {0, 255, 242};
    }

    cv::RNG& rng = cv::theRNG();
    return T(cv::saturate_cast<typename T::value_type>(rng(256)),
             cv::saturate_cast<typename T::value_type>(rng(256)),
             cv::saturate_cast<typename T::value_type>(rng(256)));
}

// Lambda-aware parallel loop invoker based on cv::parallel_for
template<typename Body>
struct ParallelLoopInvoker : public cv::ParallelLoopBody
{
    ParallelLoopInvoker(const Body& body)
        : _body(body)
    {
    }

    void operator()(const cv::Range& range) const override
    {
        _body(range);
    }

private:
    Body _body;
};

template<typename Body>
void parallel_for(const cv::Range& range, const Body& body)
{
    ParallelLoopInvoker<Body> loopInvoker(body);
    cv::parallel_for_(range, loopInvoker);
}
}
