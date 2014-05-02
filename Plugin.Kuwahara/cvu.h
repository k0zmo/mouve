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

#include <opencv2/core/core.hpp>

namespace cvu
{
void KuwaharaFilter(cv::InputArray src, cv::OutputArray dst, int radius);
// Based on paper Artistic edge and corner enhancing smoothing, G. Papari, N. Petkov, P. Campisi
void generalizedKuwaharaFilter(cv::InputArray src_, cv::OutputArray dst_,
                               int radius = 6, int N = 8,
                               float smoothing = 0.3333f);
// Based on paper Image and Video Abstraction by Anisotropic Kuwahara Filtering,
// J. E. Kyprianidis, H. Kang, J. Dollner
void anisotropicKuwaharaFilter(cv::InputArray src_, cv::OutputArray dst_,
                               int radius = 6, int N = 8,
                               float smoothing = 0.3333f);

void getGeneralizedKuwaharaKernel(cv::OutputArray kernel, int N, float smoothing);

}