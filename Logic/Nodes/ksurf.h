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

#include <opencv2/features2d/features2d.hpp>

struct KeyPoint
{
    float x, y;
    float scale;
    float response;
    float orientation;
    int laplacian;
};

std::vector<KeyPoint> fastHessianDetector(const cv::Mat& intImage, 
                                          double hessianThreshold = 40.0,
                                          int numOctaves = 4, 
                                          int numScales = 4,
                                          int initSampling = 1,
                                          int nFeatures = 0);

void surfOrientation(std::vector<KeyPoint>& kpoints, const cv::Mat& intImage);
void surfDescriptors(const std::vector<KeyPoint>& kpoints, const cv::Mat& intImage, cv::Mat& descriptors);
void msurfDescriptors(const std::vector<KeyPoint>& kpoints, const cv::Mat& intImage, cv::Mat& descriptors);

std::vector<cv::KeyPoint> transformKeyPoint(const std::vector<KeyPoint>& keypoints);
std::vector<KeyPoint> transformKeyPoint(const std::vector<cv::KeyPoint>& keypoints);

class kSURF : public cv::Feature2D
{
public:
    kSURF();
    kSURF(double hessianThreshold, 
        int numOctaves = 4,
        int numScales = 4,
        int initSampling = 1,
        bool mSurfDescriptor = true,
        bool upright = false);

    int descriptorSize() const;
    int descriptorType() const;

    void operator()(cv::InputArray img, cv::InputArray mask,
        std::vector<cv::KeyPoint>& keypoints) const;

    void operator()(cv::InputArray img, cv::InputArray mask,
        std::vector<cv::KeyPoint>& keypoints,
        cv::OutputArray descriptors, 
        bool useProvidedKeypoints=false) const;

protected:
    void detectImpl(const cv::Mat& image, std::vector<cv::KeyPoint>& keypoints, const cv::Mat& mask=cv::Mat()) const;
    void computeImpl(const cv::Mat& image, std::vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors) const;

private:
    double _hessianThreshold;
    int _nOctaves;
    int _nScales;
    int _initSampling;
    bool _msurf;
    bool _upright;
};

