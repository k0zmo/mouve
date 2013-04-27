#pragma once

#include <opencv2/features2d/features2d.hpp>

struct KeyPoint
{
    float x, y;
    float scale;
    float response;
    int laplacian;
    float orientation;
};

std::vector<KeyPoint> fastHessianDetector(const cv::Mat& intImage, 
                                          double hessianThreshold = 40.0,
                                          int numOctaves = 4, 
                                          int numScales = 4,
                                          int initSampling = 1,
                                          int nFeatures = 0);

void surfOrientation(std::vector<KeyPoint>& kpoints, const cv::Mat& intImage);
void surfDescriptors(const std::vector<KeyPoint>& kpoints, const cv::Mat& intImage, cv::Mat& descriptors);

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
    bool _upright;
};

