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

cv::Mat predefinedConvolutionKernel(EPredefinedConvolutionType type)
{
    switch(type)
    {
    case EPredefinedConvolutionType::NoOperation:
        return cv::Mat((cv::Mat_<double>(3,3) << 0,0,0, 0,1,0, 0,0,0));
    case EPredefinedConvolutionType::Average:
        return cv::Mat(3, 3, CV_64FC1, cv::Scalar(0.11111111));
    case EPredefinedConvolutionType::Gauss:
        return cv::Mat((cv::Mat_<double>(3,3) << 0.0625,0.125,0.0625, 0.125,0.25,0.125, 0.0625,0.125,0.0625));
    case EPredefinedConvolutionType::MeanRemoval:
        return cv::Mat((cv::Mat_<double>(3,3) << -1,-1,-1, -1,9,-1, -1,-1,-1));
    case EPredefinedConvolutionType::RobertsCross45:
        return cv::Mat((cv::Mat_<double>(3,3) << 0,0,0, 0,1,0, 0,0,-1));
    case EPredefinedConvolutionType::RobertsCross135:
        return cv::Mat((cv::Mat_<double>(3,3) << 0,0,0, 0,0,1, 0,-1,0));
    case EPredefinedConvolutionType::Laplacian:
        return cv::Mat((cv::Mat_<double>(3,3) << 0,-1,0, -1,4,-1, 0,-1,0));
    case EPredefinedConvolutionType::PrewittHorizontal:
        return cv::Mat((cv::Mat_<double>(3,3) << 1,1,1, 0,0,0, -1,-1,-1));
    case EPredefinedConvolutionType::PrewittVertical:
        return cv::Mat((cv::Mat_<double>(3,3) << -1,0,1, -1,0,1, -1,0,1));
    case EPredefinedConvolutionType::SobelHorizontal:
        return cv::Mat((cv::Mat_<double>(3,3) << -1,-2,-1, 0,0,0, 1,2,1));
    case EPredefinedConvolutionType::SobelVertical:
        return cv::Mat((cv::Mat_<double>(3,3) << -1,0,1, -2,0,2, -1,0,1));
    case EPredefinedConvolutionType::ScharrHorizontal:
        return cv::Mat((cv::Mat_<double>(3,3) << -3,-10,-3, 0,0,0, 3,10,3));
    case EPredefinedConvolutionType::ScharrVertical:
        return cv::Mat((cv::Mat_<double>(3,3) << -3,0,3, -10,0,10, -3,0,3));
    default:
        return cv::Mat(3,3,CV_64FC1,cv::Scalar(0));
    }
}

}
