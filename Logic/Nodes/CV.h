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
