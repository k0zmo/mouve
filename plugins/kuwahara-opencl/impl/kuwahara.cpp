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

#include "impl/kuwahara.h"

#include <opencv2/imgproc/imgproc.hpp>

namespace cvu {

void getGeneralizedKuwaharaKernel(cv::OutputArray kernel_, int N, float smoothing)
{
    // Create sector map
    const int kernelSize = 32;
    const float sigma = 0.25f * (kernelSize - 1);

    const float kernelRadius = kernelSize * 0.5f;
    const float kernelRadiusSquared = kernelRadius * kernelRadius;

    const float pi = 3.14159274101257f;
    const float piOverN = pi / N;

    cv::Mat& kernel = kernel_.getMatRef();
    kernel.create(kernelSize, kernelSize, CV_32FC1);
    kernel = cv::Scalar(0.0f);

    // Create simple kernel
    for (int y = 0; y < kernel.rows; ++y)
    {
        // transform from [0..height] to [-kernelRadius..kernelRadius]
        float yy = y - kernelRadius + 0.5f; // center of pixel is shifted by 0.5

        for (int x = 0; x < kernel.cols; ++x)
        {
            // transform from [0..width] to [-kernelRadius..kernelRadius]
            float xx = x - kernelRadius + 0.5f;

            // if we're still inside the circle
            if (xx * xx + yy * yy < kernelRadiusSquared)
            {
                // angle from the origin
                float argx = std::atan2(yy, xx);

                if (std::fabs(argx) <= piOverN)
                    kernel.at<float>(y, x) = 1.0f;
            }
        }
    }

    if (smoothing > 0.0f)
    {
        // Convolve with gaussian kernel
        cv::GaussianBlur(kernel, kernel, cv::Size(0, 0), smoothing * sigma);
    }

    // Multiply with gaussian kernel of the same size (gives decay effect from the origin)
    cv::Mat gaussKernel = cv::getGaussianKernel(kernelSize, sigma, CV_32F);
    gaussKernel = gaussKernel * gaussKernel.t();
    kernel = kernel.mul(gaussKernel);
}
} // namespace cvu
