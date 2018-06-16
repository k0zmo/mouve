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

#include "cvu.h"
#include "Logic/Nodes/CV.h"

#undef min
#undef max

#include <cassert>
#include <algorithm>
#include <limits>

#include <opencv2/imgproc/imgproc.hpp>

namespace std
{
template <typename T>
T clamp(T value, T a, T b)
{
    return std::min(std::max(value, a), b);
}
}

namespace cvu
{
struct ClampToEdgePolicy
{
    static int clamp(int x, int size)
    {
        //return x < 0 ? 0 : x >= width ? width - 1: x;
        return std::clamp(x, 0, size - 1);
    }
};

struct NoClampPolicy
{
    static int clamp(int x, int size)
    {
        (void)size;
        return x;
    }
};

template<typename ClampPolicy>
void KuwaharaFilter_gray_pix(int x, int y, const cv::Mat& src, cv::Mat& dst, int radius)
{
    float mean[4] = {0};
    float sqmean[4] = {0};

    const int height = src.rows;
    const int width = src.cols;

    // top-left
    for(int j = -radius; j <= 0; ++j)
    {
        for(int i = -radius; i <= 0; ++i)
        {
            float pix = static_cast<float>(src.at<uchar>(
                ClampPolicy::clamp(y + j, height),
                ClampPolicy::clamp(x + i, width)));
            mean[0] += pix;
            sqmean[0] += pix * pix;
        }
    }

    // top-right
    for(int j = -radius; j <= 0; ++j)
    {
        for(int i = 0; i <= radius; ++i)
        {
            float pix = static_cast<float>(src.at<uchar>(
                ClampPolicy::clamp(y + j, height),
                ClampPolicy::clamp(x + i, width)));
            mean[1] += pix;
            sqmean[1] += pix * pix;
        }
    }

    // bottom-right
    for(int j = 0; j <= radius; ++j)
    {
        for(int i = 0; i <= radius; ++i)
        {
            float pix = static_cast<float>(src.at<uchar>(
                ClampPolicy::clamp(y + j, height),
                ClampPolicy::clamp(x + i, width)));
            mean[2] += pix;
            sqmean[2] += pix * pix;
        }
    }

    // bottom-left
    for(int j = 0; j <= radius; ++j)
    {
        for(int i = -radius; i <= 0; ++i)
        {
            float pix = static_cast<float>(src.at<uchar>(
                ClampPolicy::clamp(y + j, height), 
                ClampPolicy::clamp(x + i, width)));
            mean[3] += pix;
            sqmean[3] += pix * pix;
        }
    }

    float n = static_cast<float>((radius + 1) * (radius + 1));
    float min_sigma = std::numeric_limits<float>::max();

    for(int k = 0; k < 4; ++k)
    {
        mean[k] /= n;
        sqmean[k] = std::abs(sqmean[k] / n -  mean[k]*mean[k]);
        if(sqmean[k] < min_sigma)
        {
            min_sigma = sqmean[k];
            dst.at<uchar>(y, x) = cv::saturate_cast<uchar>(mean[k]);
        }
    }
}

void KuwaharaFilter_gray(const cv::Mat& src, cv::Mat& dst, int radius)
{
#if 1
    cvu::parallel_for(cv::Range(radius, src.rows - radius),
        [&](const cv::Range& range)
        {
            for(int y = range.start; y < range.end; ++y)
                for(int x = radius; x < src.cols - radius; ++x)
                    KuwaharaFilter_gray_pix<cvu::NoClampPolicy>(x, y, src, dst, radius);
        });

    for(int y = 0; y < radius; ++y)
        for(int x = 0; x < src.cols; ++x)
            KuwaharaFilter_gray_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
    for(int y = src.rows - radius; y < src.rows; ++y)
        for(int x = 0; x < src.cols; ++x)
            KuwaharaFilter_gray_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
    for(int y = radius; y < src.rows - radius; ++y)
        for(int x = 0; x < radius; ++x)
            KuwaharaFilter_gray_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
    for(int y = radius; y < src.rows - radius; ++y)
        for(int x = src.cols - radius; x < src.cols; ++x)
            KuwaharaFilter_gray_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
#else
    cvu::parallel_for(cv::Range(0, src.rows), 
        [&](const cv::Range& range)
        {
            for(int y = range.start; y < range.end; ++y)
                for(int x = 0; x < src.cols; ++x)
                    KuwaharaFilter_gray_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
        });
#endif
}

template<typename ClampPolicy>
void KuwaharaFilter_bgr_pix(int x, int y, const cv::Mat& src, cv::Mat& dst, int radius)
{
    cv::Vec3f mean[4] = {0};
    cv::Vec3f sqmean[4] = {0};

    const int height = src.rows;
    const int width = src.cols;

    // top-left
    for(int j = -radius; j <= 0; ++j)
    {
        for(int i = -radius; i <= 0; ++i)
        {
            cv::Vec3f pix = static_cast<cv::Vec3f>(src.at<cv::Vec3b>(
                ClampPolicy::clamp(y + j, height),
                ClampPolicy::clamp(x + i, width)));
            mean[0] += pix;
            sqmean[0] += pix.mul(pix);
        }
    }

    // top-right
    for(int j = -radius; j <= 0; ++j)
    {
        for(int i = 0; i <= radius; ++i)
        {
            cv::Vec3f pix = static_cast<cv::Vec3f>(src.at<cv::Vec3b>(
                ClampPolicy::clamp(y + j, height),
                ClampPolicy::clamp(x + i, width)));
            mean[1] += pix;
            sqmean[1] += pix.mul(pix);
        }
    }

    // bottom-right
    for(int j = 0; j <= radius; ++j)
    {
        for(int i = 0; i <= radius; ++i)
        {
            cv::Vec3f pix = static_cast<cv::Vec3f>(src.at<cv::Vec3b>(
                ClampPolicy::clamp(y + j, height),
                ClampPolicy::clamp(x + i, width)));
            mean[2] += pix;
            sqmean[2] += pix.mul(pix);
        }
    }

    // bottom-left
    for(int j = 0; j <= radius; ++j)
    {
        for(int i = -radius; i <= 0; ++i)
        {
            cv::Vec3f pix = static_cast<cv::Vec3f>(src.at<cv::Vec3b>(
                ClampPolicy::clamp(y + j, height),
                ClampPolicy::clamp(x + i, width)));
            mean[3] += pix;
            sqmean[3] += pix.mul(pix);
        }
    }

    float n = static_cast<float>((radius + 1) * (radius + 1));
    float min_sigma = std::numeric_limits<float>::max();

    for(int k = 0; k < 4; ++k)
    {
        mean[k] /= n;
        sqmean[k] = sqmean[k] / n -  mean[k].mul(mean[k]);
        // HSI model (Lightness is the average of the three components)
        float sigma = std::abs(sqmean[k][0]) + std::abs(sqmean[k][1]) + std::abs(sqmean[k][2]);

        if(sigma < min_sigma)
        {
            min_sigma = sigma;
            dst.at<cv::Vec3b>(y, x) = mean[k];
        }
    }
}

void KuwaharaFilter_bgr(const cv::Mat& src, cv::Mat& dst, int radius)
{
#if 1
    cvu::parallel_for(cv::Range(radius, src.rows - radius), 
        [&](const cv::Range& range)
        {
            for(int y = range.start; y < range.end; ++y)
                for(int x = radius; x < src.cols - radius; ++x)
                    KuwaharaFilter_bgr_pix<cvu::NoClampPolicy>(x, y, src, dst, radius);
        });

    for(int y = 0; y < radius; ++y)
        for(int x = 0; x < src.cols; ++x)
            KuwaharaFilter_bgr_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
    for(int y = src.rows - radius; y < src.rows; ++y)
        for(int x = 0; x < src.cols; ++x)
            KuwaharaFilter_bgr_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
    for(int y = radius; y < src.rows - radius; ++y)
        for(int x = 0; x < radius; ++x)
            KuwaharaFilter_bgr_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
    for(int y = radius; y < src.rows - radius; ++y)
        for(int x = src.cols - radius; x < src.cols; ++x)
            KuwaharaFilter_bgr_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
#else
    cvu::parallel_for(cv::Range(0, src.rows),
        [&](const cv::Range& range)
        {
            for(int y = range.start; y < range.end; ++y)
                for(int x = 0; x < src.cols; ++x)
                    KuwaharaFilter_bgr_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
        });
#endif
}

void KuwaharaFilter(cv::InputArray src_, cv::OutputArray dst_, int radius)
{
    cv::Mat src  = src_.getMat();

    CV_Assert(src.depth() == CV_8U);
    CV_Assert(src.channels() == 3 || src.channels() == 1);

    cv::Mat& dst = dst_.getMatRef();
    dst.create(src.size(), src.type());

    if(src.type() == CV_8UC1)
        KuwaharaFilter_gray(src, dst, radius);
    else if(src.type() == CV_8UC3)
        KuwaharaFilter_bgr(src, dst, radius);
    else
        CV_Assert(false);
}

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
    for(int y = 0; y < kernel.rows; ++y)
    {
        // transform from [0..height] to [-kernelRadius..kernelRadius]
        float yy = y - kernelRadius + 0.5f; // center of pixel is shifted by 0.5

        for(int x = 0; x < kernel.cols; ++x)
        {
            // transform from [0..width] to [-kernelRadius..kernelRadius]
            float xx = x - kernelRadius + 0.5f;

            // if we're still inside the circle
            if(xx*xx + yy*yy < kernelRadiusSquared)
            {
                // angle from the origin
                float argx = std::atan2(yy, xx);

                if(std::fabs(argx) <= piOverN)
                    kernel.at<float>(y, x) = 1.0f;
            }
        }
    }

    if(smoothing > 0.0f)
    {
        // Convolve with gaussian kernel
        cv::GaussianBlur(kernel, kernel, cv::Size(0, 0), smoothing * sigma);
    }

    // Multiply with gaussian kernel of the same size (gives decay effect from the origin)
    cv::Mat gaussKernel = cv::getGaussianKernel(kernelSize, sigma, CV_32F);
    gaussKernel = gaussKernel * gaussKernel.t();
    kernel = kernel.mul(gaussKernel);
}

void generalizedKuwaharaFilter_gray(const cv::Mat& src, cv::Mat& dst,
                                    int radius, int N, float q, 
                                    const cv::Mat& kernel)
{
    const int radiusSquared = radius * radius;
    const float pi = 3.14159274101257f;
    const float piN = 2.0f * pi / float(N);
    const float cosPI = std::cos(piN);
    const float sinPI = std::sin(piN);

    cvu::parallel_for(cv::Range(0, src.rows), 
        [&](const cv::Range& range)
        {
            const int maxN = 8;

            for(int y = range.start; y < range.end; ++y)
            {
                for(int x = 0; x < src.cols; ++x)
                {
                    // mean, stddev and weight
                    float mean[maxN] = {0}, sqMean[maxN] = {0}, weight[maxN] = {0};

                    for(int j = -radius; j <= radius; ++j)
                    {
                        for(int i = -radius; i <= radius; ++i)
                        {
                            // outside the circle (but inside the square)
                            if(i*i + j*j > radiusSquared)
                                continue;

                            // use half-identity circle coordinates
                            float v0 = 0.5f * i / float(radius);
                            float v1 = 0.5f * j / float(radius);

                            float c = src.at<uchar>(
                                std::clamp(y + j, 0, src.rows - 1),
                                std::clamp(x + i, 0, src.cols - 1));
                            float cc = c * c;

                            for(int k = 0; k < N; ++k)
                            {
                                // map coordinates from circle of radius=radius to kernel map of radius=kernelSize
                                int u = static_cast<int>((v0 + 0.5f) * (kernel.cols - 1) + 0.5f);
                                int v = static_cast<int>((v1 + 0.5f) * (kernel.rows - 1) + 0.5f);

                                float w = kernel.at<float>(v, u);

                                weight[k] += w;
                                mean[k]   += c * w;
                                sqMean[k] += cc * w;

                                // instead of using 8 sector maps we use one and rotate coordinates
                                float v0_ =  v0*cosPI + v1*sinPI;
                                float v1_ = -v0*sinPI + v1*cosPI;
                                v0 = v0_;
                                v1 = v1_;
                            }
                        }
                    }

                    float out = 0.0f;
                    float sumWeight = 0.0f;

                    for (int k = 0; k < N; ++k)
                    {
                        mean[k] /= weight[k];
                        sqMean[k] = sqMean[k] / weight[k] - mean[k] * mean[k];

                        float w = 1.0f / (1.0f + std::pow(std::abs(sqMean[k]), 0.5f * q));

                        sumWeight += w;
                        out += mean[k] * w;
                    }

                    dst.at<uchar>(y, x) = cv::saturate_cast<uchar>(out / sumWeight);
                }
            }
        });
}

void generalizedKuwaharaFilter_bgr(const cv::Mat& src, cv::Mat& dst,
                                   int radius, int N, float q, 
                                   const cv::Mat& kernel)
{
    const int radiusSquared = radius * radius;
    const float pi = 3.14159274101257f;
    const float piN = 2.0f * pi / float(N);
    const float cosPI = std::cos(piN);
    const float sinPI = std::sin(piN);

    cvu::parallel_for(cv::Range(0, src.rows), 
        [&](const cv::Range& range)
        {
            const int maxN = 8;

            for(int y = range.start; y < range.end; ++y)
            {
                for(int x = 0; x < src.cols; ++x)
                {
                    // mean, stddev and weight
                    cv::Vec3f mean[maxN], sqMean[maxN];
                    float weight[maxN] = {0};

                    for(int j = -radius; j <= radius; ++j)
                    {
                        for(int i = -radius; i <= radius; ++i)
                        {
                            // outside the circle (but inside the square)
                            if(i*i + j*j > radiusSquared)
                                continue;

                            // use half-identity circle coordinates
                            float v0 = 0.5f * i / float(radius);
                            float v1 = 0.5f * j / float(radius);

                            cv::Vec3f c = src.at<cv::Vec3b>(
                                std::clamp(y + j, 0, src.rows - 1),
                                std::clamp(x + i, 0, src.cols - 1));
                            cv::Vec3f cc = c.mul(c);

                            for(int k = 0; k < N; ++k)
                            {
                                // map coordinates from circle of radius=radius to kernel map of radius=kernelSize
                                int u = static_cast<int>((v0 + 0.5f) * (kernel.cols - 1));
                                int v = static_cast<int>((v1 + 0.5f) * (kernel.rows - 1));

                                float w = kernel.at<float>(v, u);

                                weight[k] += w;
                                mean[k]   += c * w;
                                sqMean[k] += cc * w;

                                // instead of using 8 sector maps we use one and rotate coordinates
                                float v0_ =  v0*cosPI + v1*sinPI;
                                float v1_ = -v0*sinPI + v1*cosPI;
                                v0 = v0_;
                                v1 = v1_;
                            }
                        }
                    }

                    cv::Vec3f out;
                    float sumWeight = 0.0f;

                    for (int k = 0; k < N; ++k)
                    {
                        mean[k] /= weight[k];
                        sqMean[k] = sqMean[k] / weight[k] - mean[k].mul(mean[k]);

                        float sigmaSum = std::abs(sqMean[k][0]) + std::abs(sqMean[k][1]) + std::abs(sqMean[k][2]);
                        float w = 1.0f / (1.0f + std::pow(sigmaSum, 0.5f * q));

                        sumWeight += w;
                        out += mean[k] * w;
                    }

                    dst.at<cv::Vec3b>(y, x) = out / sumWeight;
                }
            }
        });
}

void generalizedKuwaharaFilter(cv::InputArray src_, cv::OutputArray dst_,
                               int radius, int N, float smoothing)
{
    cv::Mat src  = src_.getMat();
    const float q = 8.0f;
    const int maxN = 8;

    CV_Assert(src.depth() == CV_8U);
    CV_Assert(src.channels() == 3 || src.channels() == 1);
    CV_Assert(radius >= 1 && radius <= 20);
    CV_Assert(smoothing <= 1.0f && smoothing >= 0.0f);
    CV_Assert(N >= 3 && N <= maxN);

    cv::Mat& dst = dst_.getMatRef();
    dst.create(src.size(), src.type());

    cv::Mat kernel;
    getGeneralizedKuwaharaKernel(kernel, N, smoothing);

    if(src.type() == CV_8UC1)
        generalizedKuwaharaFilter_gray(src, dst, radius, N, q, kernel);
    else if(src.type() == CV_8UC3)
        generalizedKuwaharaFilter_bgr(src, dst, radius, N, q, kernel);
    else
        CV_Assert(false);
}

void calcOrientationAndAnisotropy(const cv::Mat& ssm, cv::Mat& oriAni)
{
    cvu::parallel_for(cv::Range(0, ssm.rows),
        [&](const cv::Range& range)
        {
            for(int y = range.start; y < range.end; ++y)
            {
                for(int x = 0; x < ssm.cols; ++x)
                {
                    cv::Vec3f g = ssm.at<cv::Vec3f>(y, x);

                    float a = g[0]-g[1];
                    float sqrt_ = sqrtf(a*a + 4*g[2]*g[2]);
                    float lambda1 = 0.5f * (g[0] + g[1] + sqrt_);
                    float lambda2 = 0.5f * (g[0] + g[1] - sqrt_);

                    float tx = lambda1 - g[0];
                    float ty = -g[2];

                    cv::Vec2f t(tx, ty);
                    if(tx*tx + ty*ty > 0)
                        t = cv::normalize(t);
                    else
                        t = cv::Vec2f(0.0, 1.0);

                    float phi = std::atan2(t[1], t[0]);
                    float A = (lambda1 + lambda2 > 0) ?
                        (lambda1 - lambda2) / (lambda1 + lambda2) : 0.0f;

                    oriAni.at<cv::Vec2f>(y, x) = cv::Vec2f(phi, A);
                }
            }
        });
}

void anisotropicKuwaharaFilter_gray(const cv::Mat& src, cv::Mat& dst,
                                    int radius, int N, float q, 
                                    float alpha, float sigmaSst,
                                    const cv::Mat& kernel)
{
    cv::Mat srcNorm;
    cv::normalize(src, srcNorm, 0, 1.0, cv::NORM_MINMAX, CV_MAKETYPE(CV_32F, src.channels()));

    const cv::Mat dx = cv::Mat((cv::Mat_<double>(3,3) << -1,0,1, -2,0,2, -1,0,1));
    const cv::Mat dy = cv::Mat((cv::Mat_<double>(3,3) << -1,-2,-1, 0,0,0, 1,2,1));

    // Calculate structure tensor (second order matrix)
    cv::Mat tmpx, tmpy;
    cv::filter2D(srcNorm, tmpx, CV_32F, dx);
    cv::filter2D(srcNorm, tmpy, CV_32F, dy);

    cv::Mat ssm(srcNorm.size(), CV_32FC3);
    cvu::parallel_for(cv::Range(0, ssm.rows),
        [&](const cv::Range& range)
        {
            for(int y = range.start; y < range.end; ++y)
            {
                for(int x = 0; x < ssm.cols; ++x)
                {
                    float x_color = tmpx.at<float>(y, x);
                    float y_color = tmpy.at<float>(y, x);

                    float fxfx = x_color * x_color;
                    float fyfy = y_color * y_color;
                    float fxfy = x_color * y_color;

                    ssm.at<cv::Vec3f>(y, x) = cv::Vec3f(fxfx, fyfy, fxfy);
                }
            }
        });

    // Smooth it
    cv::GaussianBlur(ssm, ssm, cv::Size(0,0), sigmaSst);

    cv::Mat oriAni(srcNorm.size(), CV_32FC2);
    calcOrientationAndAnisotropy(ssm, oriAni);

    const float pi = 3.14159274101257f;
    const float piN = 2.0f * pi / float(N);
    const float cosPI = std::cos(piN);
    const float sinPI = std::sin(piN);

    cvu::parallel_for(cv::Range(0, dst.rows), 
        [&](const cv::Range& range)
        {
            const int N = 8;
            for(int y = range.start; y < range.end; ++y)
            {
                for(int x = 0; x < dst.cols; ++x)
                {
                    cv::Vec2f t = oriAni.at<cv::Vec2f>(y, x);

                    float a = radius * std::clamp((alpha + t[1]) / alpha, 0.1f, 2.0f);
                    float b = radius * std::clamp(alpha / (alpha + t[1]), 0.1f, 2.0f);

                    float cos_phi = std::cos(t[0]);
                    float sin_phi = std::sin(t[0]);

                    float sr0 =  0.5f/a * cos_phi;
                    float sr1 =  0.5f/a * sin_phi;
                    float sr2 = -0.5f/b * sin_phi;
                    float sr3 =  0.5f/b * cos_phi;

                    int max_x = int(std::sqrt(a*a * cos_phi*cos_phi +
                                              b*b * sin_phi*sin_phi));
                    int max_y = int(std::sqrt(a*a * sin_phi*sin_phi +
                                              b*b * cos_phi*cos_phi));


                    float mean[N] = {0}, sqMean[N] = {0}, weight[N] = {0};

                    for (int j = -max_y; j <= max_y; ++j)
                    {
                        for (int i = -max_x; i <= max_x; ++i)
                        {
                            float v0 = sr0*i + sr1*j;
                            float v1 = sr2*i + sr3*j;

                            if((v0*v0 + v1*v1) < 0.25f)
                            {
                                float c = src.at<uchar>(
                                    std::clamp(y + j, 0, src.rows - 1),
                                    std::clamp(x + i, 0, src.cols - 1));
                                float cc = c * c;

                                for (int k = 0; k < N; ++k)
                                {
                                    int u = static_cast<int>((v0 + 0.5f) * (kernel.cols - 1));
                                    int v = static_cast<int>((v1 + 0.5f) * (kernel.rows - 1));

                                    float w = kernel.at<float>(v, u);

                                    weight[k] += w;
                                    mean[k]   += c * w;
                                    sqMean[k] += cc * w;

                                    float v0_ =  v0*cosPI + v1*sinPI;
                                    float v1_ = -v0*sinPI + v1*cosPI;
                                    v0 = v0_;
                                    v1 = v1_;
                                }
                            }
                        }
                    }

                    float out = 0.0f;
                    float sumWeight = 0.0f;

                    for (int k = 0; k < N; ++k)
                    {
                        mean[k] /= weight[k];
                        sqMean[k] = sqMean[k] / weight[k] - mean[k] * mean[k];

                        float w = 1.0f / (1.0f + std::pow(std::abs(sqMean[k]), 0.5f * q));

                        sumWeight += w;
                        out += mean[k] * w;
                    }

                    dst.at<uchar>(y, x) = cv::saturate_cast<uchar>(out / sumWeight);
                }
            }
        });
}

void anisotropicKuwaharaFilter_bgr(const cv::Mat& src, cv::Mat& dst,
                                   int radius, int N, float q, 
                                   float alpha, float sigmaSst,
                                   const cv::Mat& kernel)
{
    cv::Mat srcNorm;
    cv::normalize(src, srcNorm, 0, 1.0, cv::NORM_MINMAX, CV_MAKETYPE(CV_32F, src.channels()));

    const cv::Mat dx = cv::Mat((cv::Mat_<double>(3,3) << -1,0,1, -2,0,2, -1,0,1));
    const cv::Mat dy = cv::Mat((cv::Mat_<double>(3,3) << -1,-2,-1, 0,0,0, 1,2,1));

    // Calculate structure tensor (second order matrix)
    cv::Mat tmpx, tmpy;
    cv::filter2D(srcNorm, tmpx, CV_32F, dx);
    cv::filter2D(srcNorm, tmpy, CV_32F, dy);

    cv::Mat ssm(srcNorm.size(), CV_32FC3);
    cvu::parallel_for(cv::Range(0, ssm.rows),
        [&](const cv::Range& range)
        {
            for(int y = range.start; y < range.end; ++y)
            {
                for(int x = 0; x < ssm.cols; ++x)
                {
                    cv::Vec3f x_rgb = tmpx.at<cv::Vec3f>(y, x);
                    cv::Vec3f y_rgb = tmpy.at<cv::Vec3f>(y, x);

                    float fxfx = x_rgb.dot(x_rgb);
                    float fyfy = y_rgb.dot(y_rgb);
                    float fxfy = x_rgb.dot(y_rgb);

                    ssm.at<cv::Vec3f>(y, x) = cv::Vec3f(fxfx, fyfy, fxfy);
                }
            }
        });

    // Smooth it
    cv::GaussianBlur(ssm, ssm, cv::Size(0,0), sigmaSst);

    cv::Mat oriAni(srcNorm.size(), CV_32FC2);
    calcOrientationAndAnisotropy(ssm, oriAni);
    
    const float pi = 3.14159274101257f;
    const float piN = 2.0f * pi / float(N);
    const float cosPI = std::cos(piN);
    const float sinPI = std::sin(piN);

    cvu::parallel_for(cv::Range(0, dst.rows), 
        [&](const cv::Range& range)
        {
            const int N = 8;
            for(int y = range.start; y < range.end; ++y)
            {
                for(int x = 0; x < dst.cols; ++x)
                {
                    cv::Vec2f t = oriAni.at<cv::Vec2f>(y, x);

                    float a = radius * std::clamp((alpha + t[1]) / alpha, 0.1f, 2.0f);
                    float b = radius * std::clamp(alpha / (alpha + t[1]), 0.1f, 2.0f);

                    float cos_phi = std::cos(t[0]);
                    float sin_phi = std::sin(t[0]);

                    float sr0 =  0.5f/a * cos_phi;
                    float sr1 =  0.5f/a * sin_phi;
                    float sr2 = -0.5f/b * sin_phi;
                    float sr3 =  0.5f/b * cos_phi;

                    int max_x = int(std::sqrt(a*a * cos_phi*cos_phi +
                                              b*b * sin_phi*sin_phi));
                    int max_y = int(std::sqrt(a*a * sin_phi*sin_phi +
                                              b*b * cos_phi*cos_phi));


                    cv::Vec3f mean[N], sqMean[N];
                    float weight[N] = {0};

                    for (int j = -max_y; j <= max_y; ++j)
                    {
                        for (int i = -max_x; i <= max_x; ++i)
                        {
                            float v0 = sr0*i + sr1*j;
                            float v1 = sr2*i + sr3*j;

                            if((v0*v0 + v1*v1) < 0.25f)
                            {
                                cv::Vec3f c = src.at<cv::Vec3b>(
                                    std::clamp(y + j, 0, src.rows - 1),
                                    std::clamp(x + i, 0, src.cols - 1));
                                cv::Vec3f cc = c.mul(c);

                                for (int k = 0; k < N; ++k)
                                {
                                    int u = static_cast<int>((v0 + 0.5f) * (kernel.cols - 1));
                                    int v = static_cast<int>((v1 + 0.5f) * (kernel.rows - 1));

                                    float w = kernel.at<float>(v, u);

                                    weight[k] += w;
                                    mean[k]   += c * w;
                                    sqMean[k] += cc * w;

                                    float v0_ =  v0*cosPI + v1*sinPI;
                                    float v1_ = -v0*sinPI + v1*cosPI;
                                    v0 = v0_;
                                    v1 = v1_;
                                }
                            }
                        }
                    }

                    cv::Vec3f out;
                    float sumWeight = 0.0f;

                    for (int k = 0; k < N; ++k)
                    {
                        mean[k] /= weight[k];
                        sqMean[k] = sqMean[k] / weight[k] - mean[k].mul(mean[k]);

                        float sigma2 = std::abs(sqMean[k][0]) + std::abs(sqMean[k][1]) + std::abs(sqMean[k][2]);
                        float w = 1.0f / (1.0f + std::pow(sigma2, 0.5f * q));

                        sumWeight += w;
                        out += mean[k] * w;
                    }

                    dst.at<cv::Vec3b>(y, x) = out / sumWeight;
                }
            }
        });
}

void anisotropicKuwaharaFilter(cv::InputArray src_, cv::OutputArray dst_,
                               int radius, int N, float smoothing)
{
    cv::Mat src  = src_.getMat();
    const float q = 8.0f;
    const int maxN = 8;
    const float sigmaSst = 2.0f;
    const float alpha = 1.0f;

    CV_Assert(src.depth() == CV_8U);
    CV_Assert(src.channels() == 3 || src.channels() == 1);
    CV_Assert(radius >= 1 && radius <= 20);
    CV_Assert(smoothing <= 1.0f && smoothing >= 0.0f);
    CV_Assert(N >= 3 && N <= maxN);

    cv::Mat& dst = dst_.getMatRef();
    dst.create(src.size(), CV_MAKETYPE(CV_8U, src.channels()));

    cv::Mat kernel;
    getGeneralizedKuwaharaKernel(kernel, N, smoothing);

    if(src.type() == CV_8UC1)
        anisotropicKuwaharaFilter_gray(src, dst, radius, N, q, alpha, sigmaSst, kernel);
    else if(src.type() == CV_8UC3)
        anisotropicKuwaharaFilter_bgr(src, dst, radius, N, q, alpha, sigmaSst, kernel);
    else
        CV_Assert(false);
}

}