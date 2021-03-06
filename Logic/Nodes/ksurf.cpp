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

#include "ksurf.h"

#include <opencv2/imgproc/imgproc.hpp>

#if defined(HAVE_TBB)
#  include <tbb/tbb.h>
#endif

using namespace std;

// Interpolate hessian value across (x,y,s) using taylor series instead of 2nd order polynomial fit
#define INTERPOLATE_HESSIAN 0
// Use modified haar wavelets (gradient approximation)
#define SYMETRIC_HAAR 1
// Use exactly the same filters sizes as H. Bay in his paper
#define FILTER_SIZE_BAY 1

struct float2
{
    float x, y;
};

#if !defined(M_PI)
#  define  M_PI 3.141592653589793115998
#endif

#if !defined(M_PI_F)
#  define  M_PI_F 3.14159274101257f
#endif

#define RAD2DEG(x) (x * 180.0f/M_PI_F)
#define DEG2RAD(x) (x * M_PI_F/180.0f)

// Coefficient used to transform sigma size of Gaussian filter to its box filter approximation
static const float SCALE_TO_BOXSIZE_COEFF = 9.0f/1.2f;
// Coefficient used to transform box filter size to its sigma approximation
static const float BOXSIZE_TO_SCALE_COEFF = 1.2f/9.0f;

// Size of a 1st box filter in 1st octave (9x9 is default)
static const int FILTER_SIZE_BASE = 9;
static const int FILTER_SIZE_BASE_INCREASE = 6;

static const float ORIENTATION_WINDOW_SIZE = M_PI_F/3.0f;
static const float ORIENTATION_WINDOW_STEP = M_PI_F/32.0f;

static const int BASIC_DESC_SIZE = 20;
static const int BASIC_DESC_SUBREGION_SAMPLES = 5;
static const float BASIC_DESC_SIGMA = 3.3f;

// We assume there's always 4x4 subregions that makes a descriptor region
static const int DESC_OVERLAP = 2;
static const int DESC_SIZE = 20;
static const int DESC_SUBREGION_UNIQUE_SAMPLES = 5;
static const int DESC_SUBREGION_SAMPLES = DESC_SUBREGION_UNIQUE_SAMPLES+2*DESC_OVERLAP;
static const float DESC_SUBREGION_SIGMA = 2.5f;
static const float DESC_REGION_SIGMA = 1.5f;
static const int DESC_REGION_CACHE_WIDTH = (DESC_SIZE+DESC_OVERLAP*2);
static const int DESC_REGION_CACHE_SIZE = DESC_REGION_CACHE_WIDTH*DESC_REGION_CACHE_WIDTH;

struct ScaleSpaceLayer
{
    // determinant of Hesse's matrix
    vector<float> hessian; 
    // trace (Lxx + Lyy) of Hesse's matrix - which is Laplacian
    vector<float> laplacian;
    // Width of the scale space layer
    int width;
    // Height of the scale space layer
    int height;
    // Sample step - equals to 1 or multiply of 2
    int sampleStep;
    // Size of box filter this layer was blurred with
    int filterSize;

    inline float readHessian(int x, int y) const { return hessian[x + y * width]; }
    inline float readLaplacian(int x, int y) const { return laplacian[x + y * width]; }
    inline void writeHessian(int x, int y, float H) { hessian[x + y * width] = H; }
    inline void writeLaplacian(int x, int y, float L) { laplacian[x + y * width] = L; }

    void readNeighbour(int x, int y, float N9[9]) const
    {
        const float* ptr = &hessian[x + y * width];
        N9[0] = ptr[-width - 1];
        N9[1] = ptr[-width];
        N9[2] = ptr[-width + 1];

        N9[3] = ptr[-1];
        N9[4] = ptr[0];
        N9[5] = ptr[+1];

        N9[6] = ptr[+width - 1];
        N9[7] = ptr[+width];
        N9[8] = ptr[+width + 1];
    }
};

vector<cv::KeyPoint> transformKeyPoint(const vector<KeyPoint>& keypoints)
{
#if defined(HAVE_TBB)
    vector<cv::KeyPoint> ret(keypoints.size());

    tbb::parallel_for(0, (int) keypoints.size(), [&](int idx) {
        const auto& kp = keypoints[idx];
        ret[idx] = cv::KeyPoint(kp.x, kp.y, kp.scale,
            RAD2DEG(kp.orientation), kp.response, 0 /*octave*/, kp.laplacian);
    });

    return ret;
#else
    vector<cv::KeyPoint> ret;

    transform(begin(keypoints), end(keypoints), 
        back_inserter(ret), [](const KeyPoint& kp) -> cv::KeyPoint
    {
        return cv::KeyPoint(kp.x, kp.y, kp.scale,
            RAD2DEG(kp.orientation), kp.response, 0 /*octave*/, kp.laplacian);
    });

    return ret;
#endif
}

vector<KeyPoint> transformKeyPoint(const vector<cv::KeyPoint>& keypoints)
{
#if defined(HAVE_TBB)
    vector<KeyPoint> ret(keypoints.size());

    tbb::parallel_for(0, (int) keypoints.size(), [&](int idx) {
        const auto& kp = keypoints[idx];
        KeyPoint kpoint = {
            kp.pt.x,  kp.pt.y, 
            kp.size,
            kp.response,
            DEG2RAD(kp.angle),
            kp.class_id
        };
        ret[idx] = kpoint;
    });
    return ret;
#else
    vector<KeyPoint> ret;

    transform(begin(keypoints), end(keypoints), 
        back_inserter(ret), [](const cv::KeyPoint& kp) -> KeyPoint
    {
        KeyPoint kpoint = {
            kp.pt.x,  kp.pt.y, 
            kp.size,
            kp.response,
            DEG2RAD(kp.angle),
            kp.class_id
        };
        return kpoint;
    });

    return ret;
#endif
}

// Tu podaje sie srodek i promienie filtru
static inline int boxFilterIntegral(const cv::Mat& integral, int x, int y, int w, int h)
{
    int A = integral.at<int>(y - h    , x - w    );
    int B = integral.at<int>(y - h    , x + w + 1);
    int C = integral.at<int>(y + h + 1, x - w    );
    int D = integral.at<int>(y + h + 1, x + w + 1);

    return max(0, A + D - B - C);
}

// Tu podaje sie top-left obszaru oraz jego rozmiar
static inline int boxFilterIntegral_(const cv::Mat& integral, int x, int y, int w, int h)
{
    int A = integral.at<int>(y    , x    );
    int B = integral.at<int>(y    , x + w);
    int C = integral.at<int>(y + h, x    );
    int D = integral.at<int>(y + h, x + w);

    return max(0, A + D - B - C);
}

template<int N/*=9*/, int M/*=3*/>
static inline bool isLocalMaximum(float v, float N9[M][N])
{
    for(int m = 0; m < M; ++m)
        for(int n = 0; n < N; ++n)
            if(v < N9[m][n])
                return false;
    return true;
}

#if INTERPOLATE_HESSIAN != 0
static cv::Vec3f solve3x3(const cv::Matx33f& A, const cv::Vec3f& b)
{
    // Cramer rule
    float det = A(0,0)*A(1,1)*A(2,2) + A(0,1)*A(1,2)*A(2,0) + A(0,2)*A(1,0)*A(2,1)
        - A(0,2)*A(1,1)*A(2,0) - A(0,1)*A(1,0)*A(2,2) - A(0,0)*A(1,2)*A(2,1);
    if(fabsf(det) < 1e-6)
        return cv::Vec3f(0, 0, 0);

    float detA = b(0)*A(1,1)*A(2,2) + A(0,1)*A(1,2)*b(2) + A(0,2)*b(1)*A(2,1)
        - A(0,2)*A(1,1)*b(2) - A(0,1)*b(1)*A(2,2) - b(0)*A(1,2)*A(2,1);
    float detB = A(0,0)*b(1)*A(2,2) + b(0)*A(1,2)*A(2,0) + A(0,2)*A(1,0)*b(2)
        - A(0,2)*b(1)*A(2,0) - b(0)*A(1,0)*A(2,2) - A(0,0)*A(1,2)*b(2);
    float detC = A(0,0)*A(1,1)*b(2) + A(0,1)*b(1)*A(2,0) + b(0)*A(1,0)*A(2,1)
        - b(0)*A(1,1)*A(2,0) - A(0,1)*A(1,0)*b(2) - A(0,0)*b(1)*A(2,1);

    float invDet = 1.0f / det;
    return cv::Vec3f(detA * invDet, detB * invDet, detC * invDet);
}
#endif

static inline float polyPeak(float lower, float middle, float upper)
{
    float a = 0.5f*lower - middle + 0.5f*upper;
    float b = 0.5f*upper - 0.5f*lower;
    
    return -b/(2.0f*a);
}

static inline float2 calcHaarResponses(const cv::Mat& intImage, int x, int y, int radius)
{
#if SYMETRIC_HAAR != 0
    int side = 2*radius+1;
    float invarea = 1.0f/(side*radius);
    float2 dxdy;
    dxdy.x = 
        + boxFilterIntegral_(intImage, x + 1     , y - radius, radius, side) * invarea 
        - boxFilterIntegral_(intImage, x - radius, y - radius, radius, side) * invarea;
    dxdy.y = 
        + boxFilterIntegral_(intImage, x - radius, y + 1     , side, radius) * invarea
        - boxFilterIntegral_(intImage, x - radius, y - radius, side, radius) * invarea;
    return dxdy;
#else
    // Optimised version - uses shared samples in Haar (6 sampels instead of 8 in direct approach)
    float invarea = 1.0f/(2*radius*radius);
    float2 dxdy;
    int A = intImage.at<int>(y - radius, x - radius);
    int B = intImage.at<int>(y - radius, x         );
    int E = intImage.at<int>(y - radius, x + radius);
    int C = intImage.at<int>(y + radius, x - radius);
    int D = intImage.at<int>(y + radius, x         );
    int F = intImage.at<int>(y + radius, x + radius);
    dxdy.x = -(A + D - B - C) * invarea +(B + F - E - D) * invarea;

    int G = intImage.at<int>(y, x - radius);
    int H = intImage.at<int>(y, x + radius);
    dxdy.y = +(G + F - H - C) * invarea -(A + H - E - G) * invarea;
    return dxdy;
#endif
}

static inline bool haarInBounds(int x, int y, int grad_radius, const cv::Mat& intImage)
{
#if SYMETRIC_HAAR != 0
    if(x - grad_radius >= 0 && x + grad_radius+1 < intImage.cols
        && y - grad_radius >= 0 && y + grad_radius+1 < intImage.rows)
    {
        return true;
    }
    return false;
#else
    if(x - grad_radius >= 0 && x + grad_radius < intImage.cols
        && y - grad_radius >= 0 && y + grad_radius < intImage.rows)
    {
        return true;
    }
    return false;
#endif
}

static inline float getAngle(float x, float y)
{
    float angle = atan2f(y, x);
    if(angle < 0)
        angle += 2*M_PI_F;
    return angle;
}

static inline float getAngle(float2 xy)
{
    return getAngle(xy.x, xy.y);
}

static inline double Gaussian(int x, double sig)
{
    double sig2 = 2.0*sig*sig;
    return (1.0/(M_PI*sig2)) * exp(-(x*x)/(sig2));
}

static inline double Gaussian(int x, int y, double sig)
{
    double sig2 = 2.0*sig*sig;
    return (1.0/(M_PI*sig2)) * exp(-(x*x+y*y)/(sig2));
}

static inline double Gaussian(double x, double y, double sig)
{
    double sig2 = 2.0*sig*sig;
    return (1.0/(M_PI*sig2)) * exp(-(x*x+y*y)/(sig2));
}

static inline void buildScaleSpaceLayer_pix(ScaleSpaceLayer& scaleLayer, 
                                            const cv::Mat &intImage, 
                                            int lx, int ly, 
                                            int stepOffset, 
                                            int fw, int lw, int lh, 
                                            int xyOffset, 
                                            float invNorm)
{
    int x = lx * scaleLayer.sampleStep + stepOffset;
    int y = ly * scaleLayer.sampleStep + stepOffset;

    float Lxx = float(
        + boxFilterIntegral(intImage, x, y, fw, lh)
        - boxFilterIntegral(intImage, x, y, lw, lh) * 3);
    float Lyy = float(
        + boxFilterIntegral(intImage, x, y, lh, fw)
        - boxFilterIntegral(intImage, x, y, lh, lw) * 3);
    float Lxy = float(
        + boxFilterIntegral(intImage, x - xyOffset, y - xyOffset, lw, lw)
        - boxFilterIntegral(intImage, x + xyOffset, y - xyOffset, lw, lw)
        - boxFilterIntegral(intImage, x - xyOffset, y + xyOffset, lw, lw)
        + boxFilterIntegral(intImage, x + xyOffset, y + xyOffset, lw, lw));

    Lxx *= invNorm;
    Lyy *= invNorm;
    Lxy *= invNorm;

    // Hessian is given as Lxx*Lyy - theta*(Lxy*Lxy) (matrix det)
    float det = Lxx * Lyy - 0.81f * Lxy * Lxy;;
    // Laplacian is given as Lxx + Lyy (matrix trace)
    float trace = Lxx + Lyy;

    scaleLayer.writeHessian(lx, ly, det);
    scaleLayer.writeLaplacian(lx, ly, trace);
}

static void buildScaleSpaceLayer(ScaleSpaceLayer& scaleLayer,
                                 const cv::Mat &intImage) 
{
    if((intImage.cols - 1) < scaleLayer.filterSize
    || (intImage.rows - 1) < scaleLayer.filterSize)
        return;

    // Promien filtru: dla 9x9 jest to 4
    int fw = (scaleLayer.filterSize - 1) / 2;
    // wielkosc 'garbu' (1/3 wielkosci filtru)
    int lobe = scaleLayer.filterSize / 3;
    // Promien poziomy (dla Lxx) garbu
    int lw = (lobe - 1) / 2;
    // Promien pionowy (dla Lxx) garbu
    int lh = lobe - 1;
    // Przesuniecie od 0,0 do centra garbu dla Lxy
    int xyOffset = lw + 1;

    // Odwrotnosc pola filtru - do normalizacji
    float invNorm = 1.0f / (scaleLayer.filterSize*scaleLayer.filterSize);

    // Wielkosc ramki dla ktorej nie bedzie liczone det(H) oraz tr(H)
    int layerMargin = scaleLayer.filterSize / (2 * scaleLayer.sampleStep);

    // Moze zdarzyc sie tak (od drugiej oktawy jest tak zawsze) ze przez sub-sampling
    // zaczniemy filtrowac od zbyt malego marginesu - ta wartosc (zwykle 1) pozwala tego uniknac
    int stepOffset = fw - (layerMargin * scaleLayer.sampleStep);

    // Ilosc probek w poziomie i pionie dla danej oktawy i skali
    int samples_x = 1 + ((intImage.cols - 1) - scaleLayer.filterSize) / scaleLayer.sampleStep;
    int samples_y = 1 + ((intImage.rows - 1) - scaleLayer.filterSize) / scaleLayer.sampleStep;

    int ystart = layerMargin;
    int yend = layerMargin + samples_y;
    int xstart = layerMargin;
    int xend = layerMargin + samples_x;

#if defined(HAVE_TBB)
    tbb::parallel_for(tbb::blocked_range2d<int>(ystart, yend, xstart, xend),
        [&](const tbb::blocked_range2d<int>& range)	{
            for(int ly = range.rows().begin(); ly < range.rows().end(); ++ly)
            {
                for(int lx = range.cols().begin(); lx < range.cols().end(); ++lx)
                {
                    buildScaleSpaceLayer_pix(scaleLayer, intImage, lx, ly, 
                        stepOffset, fw, lw, lh, xyOffset, invNorm);
                }
            }
    });
#else
    for(int ly = ystart; ly < yend; ++ly)
    {
        for(int lx = xstart; lx < xend; ++lx)
        {
            buildScaleSpaceLayer_pix(scaleLayer, intImage, lx, ly, 
                stepOffset, fw, lw, lh, xyOffset, invNorm);
        }
    }
#endif
}

static inline void findScaleSpaceMaxima_pix(double hessianThreshold, int lx, int ly,
                                            const ScaleSpaceLayer& layerBottom,
                                            const ScaleSpaceLayer& layerMiddle,
                                            const ScaleSpaceLayer& layerTop,
#if defined(HAVE_TBB)
                                            tbb::concurrent_vector<KeyPoint>& kpoints)
#else
                                            vector<KeyPoint>& kpoints)
#endif
{
    // Check current pixel against given hessian threshold
    float center = layerMiddle.readHessian(lx, ly);
    if(center < hessianThreshold)
        return;

    // Get 3x3x3 neighbour of current pixel
    float N9[3][9];

    layerBottom.readNeighbour(lx, ly, N9[0]);
    layerMiddle.readNeighbour(lx, ly, N9[1]);
    layerTop.readNeighbour(lx, ly, N9[2]);

    bool localMax = 
        center > N9[0][0] && center > N9[0][1] && center > N9[0][2] && 
        center > N9[0][3] && center > N9[0][4] && center > N9[0][5] && 
        center > N9[0][6] && center > N9[0][7] && center > N9[0][8] && 

        center > N9[1][0] && center > N9[1][1] && center > N9[1][2] && 
        center > N9[1][3] &&                      center > N9[1][5] && 
        center > N9[1][6] && center > N9[1][7] && center > N9[1][8] && 

        center > N9[2][0] && center > N9[2][1] && center > N9[2][2] && 
        center > N9[2][3] && center > N9[2][4] && center > N9[2][5] && 
        center > N9[2][6] && center > N9[2][7] && center > N9[2][8];

    // And check current pixel against its local maxima
    //if(isLocalMaximum<9, 3>(center, N9))
    if(localMax)
    {
        int scaleDiff = layerMiddle.filterSize - layerBottom.filterSize;

#if INTERPOLATE_HESSIAN != 0
        // derivative of Hessian(x,y,s) (which is determinant) - Antigradient of dx, dy and ds:
        float Hx = -(N9[1][5] - N9[1][3]) * 0.5f;
        float Hy = -(N9[1][7] - N9[1][1]) * 0.5f;
        float Hs = -(N9[2][4] - N9[0][4]) * 0.5f;
        cv::Vec3f b(Hx, Hy, Hs);

        // Equations for calculating finite differences of second order function with N variables:
        // http://en.wikipedia.org/wiki/Finite_difference

        // second derivative of H(x,y,s) - Hessian (I know - silly)
        float Hxx = N9[1][5] - 2 * N9[1][4] + N9[1][3];
        float Hxy = (N9[1][8] - N9[1][2] - N9[1][6] + N9[1][0]) * 0.25f;
        float Hxs = (N9[2][5] - N9[0][5] - N9[2][3] + N9[0][3]) * 0.25f;
        float Hyy = N9[1][7] - 2 * N9[1][4] + N9[1][1];
        float Hys = (N9[2][7] - N9[0][7] - N9[2][1] + N9[0][1]) * 0.25f;
        float Hss = N9[2][4] - 2 * N9[1][4] + N9[0][4];
        cv::Matx33f A(Hxx, Hxy, Hxs,
            Hxy, Hyy, Hys,
            Hxs, Hys, Hss);

        // For small linear system finding a solution is much faster using cramer's rule
        cv::Vec3f x = solve3x3(A,b);
        //cv::Vec3f x = A.solve(b, cv::DECOMP_LU);

        if (fabsf(x[0]) <= 0.5f && fabsf(x[1]) <= 0.5f && fabsf(x[2]) <= 0.5f)
        {
            KeyPoint kpoint;
            kpoint.x = (lx + x[0]) * layerMiddle.sampleStep;
            kpoint.y = (ly + x[1]) * layerMiddle.sampleStep;
            kpoint.scale = BOXSIZE_TO_SCALE_COEFF * (layerMiddle.filterSize + x[2] * scaleDiff);
            kpoint.response = center;
            kpoint.laplacian = (layerMiddle.readLaplacian(lx, ly) >= 0 ? 1 : -1);
            kpoints.push_back(kpoint); // <-- concurrent_vector<>
        }
#else
        float peakX = polyPeak(N9[1][3], center, N9[1][5]);
        float peakY = polyPeak(N9[1][1], center, N9[1][7]);
        float peakS = polyPeak(N9[0][4], center, N9[2][4]);

        float interpX = (lx + peakX) * layerMiddle.sampleStep;
        float interpY = (ly + peakY) * layerMiddle.sampleStep;
        float interpS = layerMiddle.filterSize + peakS*scaleDiff;

        KeyPoint kpoint;
        kpoint.x = interpX;
        kpoint.y = interpY;
        kpoint.scale = BOXSIZE_TO_SCALE_COEFF * interpS;
        kpoint.response = center;
        kpoint.laplacian = (layerMiddle.readLaplacian(lx, ly) >= 0 ? 1 : -1);
        kpoints.push_back(kpoint);
#endif
    }
}

static void findScaleSpaceMaxima(double hessianThreshold,
                                 const ScaleSpaceLayer& layerBottom,
                                 const ScaleSpaceLayer& layerMiddle,
                                 const ScaleSpaceLayer& layerTop,
                                 vector<KeyPoint>& kpoints)
{
    // Margin is one pixel bigger than during hessian
    // calculating because we need valid 3x3x3 whole neighbourhood 
    int layerMargin = (layerTop.filterSize) / (2 * layerTop.sampleStep) + 1;

    int ystart = layerMargin;
    int yend = layerMiddle.height - layerMargin;
    int xstart = layerMargin;
    int xend = layerMiddle.width - layerMargin;

#if defined(HAVE_TBB)
    tbb::concurrent_vector<KeyPoint> curr_kpoints;

    tbb::parallel_for(tbb::blocked_range2d<int>(ystart, yend, xstart, xend),
        [&](const tbb::blocked_range2d<int>& range) {
            for(int ly = range.rows().begin(); ly < range.rows().end(); ++ly) 
            {
                for(int lx = range.cols().begin(); lx < range.cols().end(); ++lx)
                {
                    findScaleSpaceMaxima_pix(hessianThreshold, lx, ly, 
                        layerBottom, layerMiddle, layerTop, curr_kpoints);
                }
            }
    });

    copy(begin(curr_kpoints), end(curr_kpoints), back_inserter(kpoints));
#else
    for(int ly = ystart; ly < yend; ++ly) 
    {
        for(int lx = xstart; lx < xend; ++lx)
        {
            findScaleSpaceMaxima_pix(hessianThreshold, lx, ly, 
                layerBottom, layerMiddle, layerTop, kpoints);
        }
    }
#endif
}

static inline void retainBestFeatures(int nFeatures, 
                                      vector<KeyPoint>& kpoints)
{
    if(nFeatures > 0)
    {
#if defined(HAVE_TBB)
        tbb::parallel_sort(begin(kpoints), end(kpoints),
            [](const KeyPoint& left, const KeyPoint& right) { 
                return left.response < right.response;
        });
#else
        sort(begin(kpoints), end(kpoints),
            [](const KeyPoint& left, const KeyPoint& right) { 
                return left.response < right.response;
        });
#endif
        if(nFeatures <= (int) kpoints.size())
            kpoints.resize(nFeatures);
    }
}

vector<KeyPoint> fastHessianDetector(const cv::Mat& intImage, 
                                     double hessianThreshold,
                                     int numOctaves, 
                                     int numScales,
                                     int initSampling,
                                     int nFeatures)
{
    CV_Assert(intImage.type() == CV_32S);
    CV_Assert(intImage.channels() == 1);
    CV_Assert(numOctaves > 0);
    CV_Assert(numScales > 2);
    CV_Assert(hessianThreshold >= 0.0f);

    int scaleBaseFilterSize = FILTER_SIZE_BASE;
    vector<KeyPoint> kpoints;

    for(int octave = 0; octave < numOctaves; ++octave)
    {
        vector<ScaleSpaceLayer> scaleLayers(numScales);

        for(int scale = 0; scale < numScales; ++scale)
        {
            auto& layer = scaleLayers[scale];

            layer.width = (intImage.cols-1) >> octave;
            layer.height = (intImage.rows-1) >> octave;
            layer.sampleStep = initSampling << octave;
#if FILTER_SIZE_BAY == 1
            layer.filterSize = scaleBaseFilterSize + (scale * FILTER_SIZE_BASE_INCREASE << octave);
#else
            layer.filterSize = (FILTER_SIZE_BASE + FILTER_SIZE_BASE_INCREASE*scale) << octave;
#endif

            // Allocate required memory for scale layers
            layer.hessian.resize(layer.width * layer.height);
            layer.laplacian.resize(layer.width * layer.height);

            buildScaleSpaceLayer(layer, intImage);
        }

        for(int scale = 1; scale < numScales - 1; ++scale)
        {
            const auto& layerBottom = scaleLayers[scale - 1];
            const auto& layerMiddle = scaleLayers[scale];
            const auto& layerTop = scaleLayers[scale + 1];

            findScaleSpaceMaxima(hessianThreshold, layerBottom, 
                layerMiddle, layerTop, kpoints);
        }

        scaleBaseFilterSize += FILTER_SIZE_BASE_INCREASE << octave;
    }

    // Sort and trim weakest features 
    retainBestFeatures(nFeatures, kpoints);

    return kpoints;
}

void surfOrientation(vector<KeyPoint>& kpoints,
                     const cv::Mat& intImage)
{
    // Orientation radius is 6*s with 2.5 sigma for gaussian
    // NOTE: In H. Bay's paper it says gradients are weighted with sigma associated with keypoint scale
    // but I didn't noticed any (positive) difference for using it - instead Gaussian with contant 
    // sigma is used
    cv::Mat gauss_ = cv::getGaussianKernel(13, 2.5, CV_32F);

    const int nOriSamples = 109;
    vector<cv::Point> oriSamples; oriSamples.reserve(nOriSamples);
    vector<float> oriWeights; oriWeights.reserve(nOriSamples);

    // Builds list of pixel coordinates to process and their Gaussian-based weight
    for(int y = -6; y <= 6; ++y)
    {
        for(int x = -6; x <= 6; ++x)
        {
            if(x*x + y*y < 36)
            {
                oriSamples.emplace_back(x, y);
                oriWeights.emplace_back(gauss_.at<float>(y+6) * gauss_.at<float>(x+6));
            }
        }
    }
    CV_Assert(oriSamples.size() == nOriSamples);
    CV_Assert(oriWeights.size() == nOriSamples);

#if defined(HAVE_TBB)
    tbb::parallel_for_each(begin(kpoints), end(kpoints),
        [&](KeyPoint& kpoint) {
#else
    for_each(begin(kpoints), end(kpoints), 
        [&](KeyPoint& kpoint) {
#endif
            struct HaarResponse
            {
                float2 dxdy;
                float angle;
            };

            vector<HaarResponse> responses;
            responses.reserve(nOriSamples);

            for(int i = 0; i < nOriSamples; ++i)
            {
                int x = cvRound(kpoint.x + oriSamples[i].x*kpoint.scale);
                int y = cvRound(kpoint.y + oriSamples[i].y*kpoint.scale);
                int grad_radius = cvRound(2*kpoint.scale);

                // If there's no full neighbourhood for this gradient - set it to 0 response
                if(!haarInBounds(x, y, grad_radius, intImage))
                    continue;

                HaarResponse haar;
                haar.dxdy = calcHaarResponses(intImage, x, y, grad_radius);
                haar.dxdy.x *= oriWeights[i];
                haar.dxdy.y *= oriWeights[i];
                haar.angle = getAngle(haar.dxdy);
                responses.emplace_back(move(haar));
            }

            float maxSumX = 0.0f, maxSumY = 0.0f, max = 0.0f;

            // Oblicz kierunek dominujacy
            for(float angleWin1 = 0.0f; angleWin1 < 2*M_PI_F; angleWin1 += ORIENTATION_WINDOW_STEP)
            {
                float sumX = 0, sumY = 0.0f;

                for(const auto& resp : responses)
                {
                    float d = resp.angle - angleWin1;
                    if(d > 0 && d < ORIENTATION_WINDOW_SIZE)
                    {
                        sumX += resp.dxdy.x;
                        sumY += resp.dxdy.y;
                    }
                }

                // Is this window better?
                float tmp = sumX*sumX + sumY*sumY;
                if(tmp > max)
                {
                    max = tmp;
                    maxSumX = sumX;
                    maxSumY = sumY;
                }
            }

            kpoint.orientation = getAngle(maxSumX, maxSumY);
    });
}

template<size_t N>
static inline void normalizeDescriptor(float desc[N])
{
    float len = 0.0f;
    for(size_t i = 0; i < N; ++i)
        len += desc[i]*desc[i];

    if(fabsf(len) > 1e-6)
    {
        // Normalize descriptor
        len = 1.0f / sqrtf(len);
        for(size_t i = 0; i < N; ++i)
            desc[i] *= len;
    }
}

static void msurfDescriptors_kpoint(const KeyPoint& kpoint, 
                                    const cv::Mat& intImage, 
                                    float desc[], float gaussWeights[])
{
    (void) gaussWeights;

    float scale = kpoint.scale;
    float c = cosf(kpoint.orientation);
    float s = sinf(kpoint.orientation);
    int count = 0;

    float gradsX[DESC_REGION_CACHE_SIZE];
    float gradsY[DESC_REGION_CACHE_SIZE];

    int index = 0;

    for(int y = -DESC_SIZE/2-DESC_OVERLAP; y < DESC_SIZE/2+DESC_OVERLAP; ++y)
    {
        float ys = y*scale;

        for(int x = -DESC_SIZE/2-DESC_OVERLAP; x < DESC_SIZE/2+DESC_OVERLAP; ++x)
        {
            float xs = x*scale;

            int sample_x = cvRound(kpoint.x + c*xs - s*ys);
            int sample_y = cvRound(kpoint.y + s*xs + c*ys);
            int grad_radius = cvRound(scale);

            float2 dxdy = {0,0};

            // If there's full neighbourhood for this gradient
            if(haarInBounds(sample_x, sample_y, grad_radius, intImage))
                dxdy = calcHaarResponses(intImage, sample_x, sample_y, grad_radius);

            gradsX[index] = dxdy.x;
            gradsY[index] = dxdy.y;
            ++index;
        }
    }

    // cy goes from -1.5 to 1.5 with 1.0 step
    float cy = -1.5f;

    for(int j = -DESC_SIZE/2; j < DESC_SIZE/2; j += DESC_SUBREGION_UNIQUE_SAMPLES)
    {
        // same goes for cx
        float cx = -1.5f;

        for(int i = -DESC_SIZE/2; i < DESC_SIZE/2; i += DESC_SUBREGION_UNIQUE_SAMPLES)
        {
            float sumDx = 0, sumDy = 0, sumADx = 0, sumADy = 0;

            for(int y = 0; y < DESC_SUBREGION_SAMPLES; ++y)
            {
                index = ((j+DESC_SIZE/2)+y)*DESC_REGION_CACHE_WIDTH + (i+DESC_SIZE/2);

                for(int x = 0; x < DESC_SUBREGION_SAMPLES; ++x)
                {
                    float gauss = (float) Gaussian(x-(DESC_SUBREGION_SAMPLES/2), 
                        y-(DESC_SUBREGION_SAMPLES/2), DESC_SUBREGION_SIGMA);
                    //float gauss = gaussWeights[x]*gaussWeights[y];

                    float dx = gauss*gradsX[index];
                    float dy = gauss*gradsY[index];
                    ++index;

                    float tdx =  c*dx + s*dy;
                    float tdy = -s*dx + c*dy;

                    sumDx += tdx;
                    sumDy += tdy;
                    sumADx += fabsf(tdx);
                    sumADy += fabsf(tdy);
                }
            }

            // These 16 Gaussians won't give us 1.0f in total (it's not normalized)
            // but that doesn't matter since at the end that we're normalizing desc[]
            float gauss = (float) Gaussian(cx, cy, DESC_REGION_SIGMA);
            desc[count++] = gauss*sumDx;
            desc[count++] = gauss*sumDy;
            desc[count++] = gauss*sumADx;
            desc[count++] = gauss*sumADy;
            cx += 1.0f;
        }
        cy += 1.0;
    }

    normalizeDescriptor<64>(desc);
}

static void surfDescriptors_kpoint(const KeyPoint& kpoint, 
                                   const cv::Mat& intImage, 
                                   float desc[])
{
    float scale = kpoint.scale;
    float c = cosf(kpoint.orientation);
    float s = sinf(kpoint.orientation);
    int count = 0;

    for(int j = -BASIC_DESC_SIZE/2; j < BASIC_DESC_SIZE/2; j += BASIC_DESC_SUBREGION_SAMPLES)
    {
        for(int x = -BASIC_DESC_SIZE/2; x < BASIC_DESC_SIZE/2; x += BASIC_DESC_SUBREGION_SAMPLES)
        {
            float sumDx = 0, sumDy = 0, sumADx = 0, sumADy = 0;

            for(int l = j; l < j + BASIC_DESC_SUBREGION_SAMPLES; ++l)
            {
                float regionY = l*scale;

                for(int k = x; k < x + BASIC_DESC_SUBREGION_SAMPLES; ++k)
                {
                    float regionX = k*scale;

                    // Calculate pixel coordinates of a sample in a feature region
                    int sample_x = cvRound(kpoint.x + (regionX*c - regionY*s));
                    int sample_y = cvRound(kpoint.y + (regionX*s + regionY*c));
                    int grad_radius = cvRound(scale);

                    // If there's no full neighbourhood for this gradient - set it to 0 response
                    if(!haarInBounds(sample_x, sample_y, grad_radius, intImage))
                        continue;

                    // Calculate weighted gradient along x and y directions
                    float2 dxdy = calcHaarResponses(intImage, sample_x, sample_y, grad_radius);
                    float weight = (float) Gaussian(k, l, BASIC_DESC_SIGMA*scale);
                    float dx = weight * dxdy.x;
                    float dy = weight * dxdy.y;

                    // Transform gradient along the local feature direction
                    float tdx =  c*dx + s*dy;
                    float tdy = -s*dx + c*dy;

                    // Sum values up
                    sumDx += tdx;
                    sumDy += tdy;
                    sumADx += fabsf(tdx);
                    sumADy += fabsf(tdy);
                }
            }

            desc[count++] = sumDx;
            desc[count++] = sumDy;
            desc[count++] = sumADx;
            desc[count++] = sumADy;
        }
    }

    normalizeDescriptor<64>(desc);
}

void msurfDescriptors(const vector<KeyPoint>& kpoints,
                      const cv::Mat& intImage,
                      cv::Mat& descriptors)
{
    if(descriptors.empty())
        descriptors.create((int) kpoints.size(), 64, CV_32F);

    float gaussWeights[DESC_SUBREGION_SAMPLES];
    float sum = 0.0f;
    for(int i = 0; i < DESC_SUBREGION_SAMPLES; ++i)
    {
        gaussWeights[i] = (float) Gaussian(i-(DESC_SUBREGION_SAMPLES/2), DESC_SUBREGION_SIGMA);
        sum += gaussWeights[i];
    }

    sum = 1.f/sum;
    for(int i = 0; i < DESC_SUBREGION_SAMPLES; ++i)
        gaussWeights[i] *= sum;

#if defined(HAVE_TBB)
    // We need idx for a index in descriptors matrix - no for_each for us this time
    tbb::parallel_for(0, (int) kpoints.size(), [&](int idx)
    {
        const auto& kpoint = kpoints[idx];
        float* desc = descriptors.ptr<float>(idx);
        msurfDescriptors_kpoint(kpoint, intImage, desc, gaussWeights);
    });
#else
    for(int idx = 0; idx < (int) kpoints.size(); ++idx)
    {
        const auto& kpoint = kpoints[idx];
        float* desc = descriptors.ptr<float>(idx);
        msurfDescriptors_kpoint(kpoint, intImage, desc, gaussWeights);
    }
#endif
}

void surfDescriptors(const vector<KeyPoint>& kpoints,
                     const cv::Mat& intImage,
                     cv::Mat& descriptors)
{
    if(descriptors.empty())
        descriptors.create((int) kpoints.size(), 64, CV_32F);

#if defined(HAVE_TBB)
    // We need idx for a index in descriptors matrix - no for_each for us this time
    tbb::parallel_for(0, (int) kpoints.size(), [&](int idx)
    {
        const auto& kpoint = kpoints[idx];
        float* desc = descriptors.ptr<float>(idx);
        surfDescriptors_kpoint(kpoint, intImage, desc);
    });
#else
    for(int idx = 0; idx < (int) kpoints.size(); ++idx)
    {
        const auto& kpoint = kpoints[idx];
        float* desc = descriptors.ptr<float>(idx);
        surfDescriptors_kpoint(kpoint, intImage, desc);
    }
#endif
}

kSURF::kSURF()
    : _hessianThreshold(40.0)
    , _nOctaves(4)
    , _nScales(4)
    , _initSampling(1)
    , _msurf(true)
    , _upright(false)
{
}

kSURF::kSURF(double hessianThreshold, int numOctaves, int numScales, int initSampling, bool mSurfDescriptor, bool upright)
    : _hessianThreshold(hessianThreshold)
    , _nOctaves(numOctaves)
    , _nScales(numScales)
    , _initSampling(initSampling)
    , _msurf(mSurfDescriptor)
    , _upright(upright)
{
}

int kSURF::descriptorSize() const { return 64; }
int kSURF::descriptorType() const { return CV_32F; }

void kSURF::operator()(cv::InputArray imgarg, cv::InputArray maskarg,
                       vector<cv::KeyPoint>& keypoints) const
{
    (*this)(imgarg, maskarg, keypoints, cv::noArray(), false);
}

void kSURF::operator()(cv::InputArray _img, cv::InputArray _mask,
                       vector<cv::KeyPoint>& keypoints,
                       cv::OutputArray _descriptors,
                       bool useProvidedKeypoints) const
{
    cv::Mat img = _img.getMat(), mask = _mask.getMat(), mask1, sum, msum;
    bool doDescriptors = _descriptors.needed();

    CV_Assert(!img.empty() && img.depth() == CV_8U);
    if( img.channels() > 1 )
        cvtColor(img, img, cv::COLOR_BGR2GRAY);

    CV_Assert(mask.empty() || (mask.type() == CV_8U && mask.size() == img.size()));
    CV_Assert(_hessianThreshold >= 0);
    CV_Assert(_nOctaves > 0);
    CV_Assert(_nScales > 0);

    cv::integral(img, sum, CV_32S);
    vector<KeyPoint> kpoints;

    if(useProvidedKeypoints)
        kpoints = transformKeyPoint(keypoints);
    else
        kpoints = fastHessianDetector(sum, _hessianThreshold, _nOctaves, _nScales, _initSampling);

    int N = (int) kpoints.size();
    if(N > 0)
    {
        if(!_upright)
        {
            surfOrientation(kpoints, sum);
        }
        else
        {
            // Set orientation to 0 for every keypoint
#if defined(HAVE_TBB)
            tbb::parallel_for_each(begin(kpoints), end(kpoints),
                [&](KeyPoint& kp) {
                    kp.orientation = 0;
            });
#else
            for_each(begin(kpoints), end(kpoints),
                [&](KeyPoint& kp) {
                    kp.orientation = 0;
            });
#endif
        }

        if(doDescriptors)
        {
            cv::Mat& descriptors = _descriptors.getMatRef();
            descriptors.create(N, 64, CV_32F);
            if(_msurf)
                msurfDescriptors(kpoints, sum, descriptors);
            else
                surfDescriptors(kpoints, sum, descriptors);
        }

        keypoints = transformKeyPoint(kpoints);
    }
    else
    {
        keypoints = vector<cv::KeyPoint>();
        if(doDescriptors)
        {
            cv::Mat& descriptors = _descriptors.getMatRef();
            descriptors = cv::Mat();
        }
    }
}

void kSURF::detectImpl(const cv::Mat& image, vector<cv::KeyPoint>& keypoints, const cv::Mat& mask) const
{
    (*this)(image, mask, keypoints, cv::noArray(), false);
}

void kSURF::computeImpl(const cv::Mat& image, vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors) const
{
    (*this)(image, cv::Mat(), keypoints, descriptors, true);
}
