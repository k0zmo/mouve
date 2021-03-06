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

static const float PI = 3.1415926535897932384626433832795f;
static const float RAD_PER_DEG = PI / 180.0f;
#define DEG2RAD(x) (x * RAD_PER_DEG)

cv::Mat rotateStructuringElement(int rotation, const cv::Mat& _element)
{
    // Rotacja elementu strukturalnego
    if(rotation == 0)
        return _element;

    rotation %= 360;

    auto rotateImage = [](const cv::Mat& source, double angle) -> cv::Mat
    {
        cv::Point2f srcCenter(source.cols/2.0f, source.rows/2.0f);
        cv::Mat rotMat = cv::getRotationMatrix2D(srcCenter, angle, 1.0f);
        cv::Mat dst;
        cv::warpAffine(source, dst, rotMat, source.size(), cv::INTER_LINEAR);
        return dst;
    };

    int s = 2 * std::max(_element.rows, _element.cols);
    int b = s/4;

    cv::Mat tmp(cv::Size(s, s), CV_8U, cv::Scalar(0));
    cv::copyMakeBorder(_element, tmp, b,b,b,b, cv::BORDER_CONSTANT);
    cv::Mat element(rotateImage(tmp, rotation));

    // Trzeba teraz wyciac niepotrzebny nadmiar pikseli ramkowych
    int top = 0, 
        bottom = element.rows, 
        left = 0,
        right = element.cols;

    // Zwraca true jesli wskazany wiersz w danej macierzy nie jest "pusty" (nie ma samych 0)
    auto checkRow = [](int row, const cv::Mat& e) -> bool
    {
        const uchar* p = e.ptr<uchar>(row);
        for(int x = 0; x < e.cols; ++x)
            if(p[x] != 0)
                return true;
        return false;
    };

    // Zwraca true jesli wskazana kolumna w danej macierzy nie jest "pusta" (nie ma samych 0)
    auto checkColumn = [](int column, const cv::Mat& e) -> bool
    {
        for(int y = 0; y < e.rows; ++y)
            if(e.at<uchar>(y, column) != 0)
                return true;
        return false;
    };

    // Kadruj gore
    for(int y = 0; y < element.rows; ++y)
    {
        if (checkRow(y, element))
            break;
        ++top;
    }

    // Kadruj dol
    for(int y = element.rows-1; y >= 0; --y)
    {
        if (checkRow(y, element))
            break;
        --bottom;
    }

    // Kadruj lewa strone
    for(int x = 0; x < element.cols; ++x)
    {
        if (checkColumn(x, element))
            break;
        ++left;
    }

    // Kadruj prawa strone
    for(int x = element.cols-1; x >= 0; --x)
    {
        if (checkColumn(x, element))
            break;
        --right;
    }

    int width = right-left;
    int height = bottom-top;

    // Zalozenie jest ze element strukturalny ma rozmiar 2n+1,
    // ponizsze dwa bloki strzega tego warunku

    if(!(width % 2))
    {
        width++; 
        // jesli wyjdziemy za zakres to zmniejsz poczatek ROI
        if(left+width > element.cols) 
            --left;
    }
    if(!(height % 2))
    {
        height++;
        // jesli wyjdziemy za zakres to zmniejsz poczatek ROI
        if(top+height > element.rows) 
            --top;
    }

    cv::Mat elem = element(cv::Rect(left, top, width, height));
    for(int y = 0; y < elem.rows; ++y)
        for(int x = 0; x < elem.cols; ++x)
            if(elem.at<uchar>(y, x) != 0)
                elem.at<uchar>(y, x) = 255;
    return elem;
}

cv::Mat standardStructuringElement(int xradius, int yradius,
    EStructuringElementType type, int rotation)
{
    cv::Point anchor(xradius, yradius);

    cv::Size elem_size(
        2 * anchor.x + 1,
        2 * anchor.y + 1);

    // OpenCV ma cos do rysowania elips ale nie spelnia oczekiwan
    // Patrz: przypadek elipsy o rozmiarze 17x31 i obrocie o 327 (wklesla)
    if(0 && type == EStructuringElementType::Ellipse)
    {
        int axis = std::max(xradius, yradius);
        cv::Size ksize(2*axis + 1, 2*axis + 1);
        cv::Mat element(ksize, CV_8U, cv::Scalar(0));

        cv::ellipse(element, cv::Point(axis, axis),
                    cv::Size(xradius, yradius), -rotation,
                    0, 360, cv::Scalar(255), -1, 8);
        return element;
    }

    if(type == EStructuringElementType::Ellipse)
    {
        // http://www.maa.org/joma/Volume8/Kalman/General.html
        //
        // (x*cos(t)+y*sin(t))^2   (x*sin(t)-y*cos(t))^2
        // --------------------- + --------------------- = 1
        //          a^2                     b^2

        double beta = -DEG2RAD(rotation);
        double sinbeta = sin(beta);
        double cosbeta = cos(beta);

        double a2 = static_cast<double>(xradius*xradius);
        double b2 = static_cast<double>(yradius*yradius);

        int axis = std::max(xradius, yradius);
        cv::Size ksize(2*axis + 1, 2*axis + 1);
        cv::Mat element(ksize, CV_8U);

        for(int y = -axis; y <= axis; ++y)
        {
            for(int x = -axis; x <= axis; ++x)
            {
                double n1 = x*cosbeta+y*sinbeta;
                double n2 = x*sinbeta-y*cosbeta;
                double lhs = (n1*n1)/a2 + (n2*n2)/b2;
                double rhs = 1;
                element.at<uchar>(y+axis, x+axis) = (lhs <= rhs ? 255 : 0);
            }
        }
        return element;
    }

    int shape;
    switch(type)
    {
    case EStructuringElementType::Rectangle: shape = cv::MORPH_RECT; break;
    case EStructuringElementType::Cross: shape = cv::MORPH_CROSS; break;
    default: return cv::Mat();
    }

    cv::Mat element = cv::getStructuringElement(shape, elem_size, anchor);
    element *= 255;
    return rotateStructuringElement(rotation, element);
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
