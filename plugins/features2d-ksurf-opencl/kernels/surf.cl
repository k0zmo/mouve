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

#if !defined(KEYPOINT_MAX)
#error("KEYPOINT_MAX must be defined")
#endif
#if !defined(WARP_SIZE)
#error("WARP_SIZE must be defined")
#endif

#if defined(cl_ext_atomic_counters_32)
#pragma OPENCL EXTENSION cl_ext_atomic_counters_32 : enable
#define counter_type counter32_t
#else
#define counter_type volatile __global int*
#endif

// TODO!
// Interpolate hessian value across (x,y,s) using taylor series instead of 2nd order polynomial fit
#define INTERPOLATE_HESSIAN 0
// Use modified haar wavelets (gradient approximation)
#define SYMETRIC_HAAR 1

// Do NOT change it - these are indices for big AoS containing keypoints data
#define KEYPOINT_X 0
#define KEYPOINT_Y 1
#define KEYPOINT_SCALE 2
#define KEYPOINT_RESPONSE 3
#define KEYPOINT_LAPLACIAN 4
#define KEYPOINT_ORIENTATION 5

// Coefficient used to transform box filter size to its sigma approximation
#define BOXSIZE_TO_SCALE_COEFF 1.2f / 9.0f
#define BASIC_DESC_SIGMA 3.3f

#define DESC_SUBREGION_SIGMA 2.5f
#define DESC_REGION_SIGMA 1.5f

//__constant sampler_t samp = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;
// On Intel SDK it crashes with CLK_ADDRESS_NONE during findKeypointOrientation()
__constant sampler_t samp =
    CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;

// Tu podaje sie srodek i promienie filtru
int boxFilterIntegral(__read_only image2d_t integral, int x, int y, int w, int h)
{
    int A = read_imageui(integral, samp, (int2)(x - w, y - h)).x;
    int B = read_imageui(integral, samp, (int2)(x + w + 1, y - h)).x;
    int C = read_imageui(integral, samp, (int2)(x - w, y + h + 1)).x;
    int D = read_imageui(integral, samp, (int2)(x + w + 1, y + h + 1)).x;

    // is max() necessary here?
    return max(0, A + D - B - C);
}

__kernel void buildScaleSpace(__read_only image2d_t integral, int samples_x, int samples_y,
                              __global float* hessian, __global float* laplacian, int pitch, int fw,
                              int lw, int lh, int xyOffset, float invNorm, int sampleStep,
                              int stepOffset)
{
    int gx = get_global_id(0);
    int gy = get_global_id(1);

    int x = gx * sampleStep + stepOffset;
    int y = gy * sampleStep + stepOffset;

    if (gx >= samples_x + get_global_offset(0) || gy >= samples_y + get_global_offset(1))
    {
        return;
    }

    float Lxx = (float)(+boxFilterIntegral(integral, x, y, fw, lh) -
                        boxFilterIntegral(integral, x, y, lw, lh) * 3);
    float Lyy = (float)(+boxFilterIntegral(integral, x, y, lh, fw) -
                        boxFilterIntegral(integral, x, y, lh, lw) * 3);
    float Lxy = (float)(+boxFilterIntegral(integral, x - xyOffset, y - xyOffset, lw, lw) -
                        boxFilterIntegral(integral, x + xyOffset, y - xyOffset, lw, lw) -
                        boxFilterIntegral(integral, x - xyOffset, y + xyOffset, lw, lw) +
                        boxFilterIntegral(integral, x + xyOffset, y + xyOffset, lw, lw));

    Lxx *= invNorm;
    Lyy *= invNorm;
    Lxy *= invNorm;

    // Hessian is given as Lxx*Lyy - theta*(Lxy*Lxy) (matrix det)
    float det = Lxx * Lyy - 0.81f * Lxy * Lxy;
    ;
    // Laplacian is given as Lxx + Lyy (matrix trace)
    float trace = Lxx + Lyy;

    hessian[mad24(gy, pitch, gx)] = det;
    laplacian[mad24(gy, pitch, gx)] = trace;
}

// Tu podaje sie srodek i promienie filtru
int boxFilterIntegral_buffer(__global uint* integral, int integral_pitch, int x, int y, int w,
                             int h)
{
    uint A = integral[mad24(y - h, integral_pitch, x - w)];
    uint B = integral[mad24(y - h, integral_pitch, x + w + 1)];
    uint C = integral[mad24(y + h + 1, integral_pitch, x - w)];
    uint D = integral[mad24(y + h + 1, integral_pitch, x + w + 1)];

    return A + D - B - C;
}

__kernel void buildScaleSpace_buffer(__global uint* integral, int integral_pitch, int samples_x,
                                     int samples_y, __global float* hessian,
                                     __global float* laplacian, int pitch, int fw, int lw, int lh,
                                     int xyOffset, float invNorm, int sampleStep, int stepOffset)
{
    int gx = get_global_id(0);
    int gy = get_global_id(1);

    int x = gx * sampleStep + stepOffset;
    int y = gy * sampleStep + stepOffset;

    if (gx >= samples_x + get_global_offset(0) || gy >= samples_y + get_global_offset(1))
    {
        return;
    }

    float Lxx = (float)(+boxFilterIntegral_buffer(integral, integral_pitch, x, y, fw, lh) -
                        boxFilterIntegral_buffer(integral, integral_pitch, x, y, lw, lh) * 3);
    Lxx *= invNorm;

    float Lyy = (float)(+boxFilterIntegral_buffer(integral, integral_pitch, x, y, lh, fw) -
                        boxFilterIntegral_buffer(integral, integral_pitch, x, y, lh, lw) * 3);
    Lyy *= invNorm;

    float Lxy = (float)(+boxFilterIntegral_buffer(integral, integral_pitch, x - xyOffset,
                                                  y - xyOffset, lw, lw) -
                        boxFilterIntegral_buffer(integral, integral_pitch, x + xyOffset,
                                                 y - xyOffset, lw, lw) -
                        boxFilterIntegral_buffer(integral, integral_pitch, x - xyOffset,
                                                 y + xyOffset, lw, lw) +
                        boxFilterIntegral_buffer(integral, integral_pitch, x + xyOffset,
                                                 y + xyOffset, lw, lw));
    Lxy *= invNorm;

    // Hessian is given as Lxx*Lyy - theta*(Lxy*Lxy) (matrix det)
    float det = Lxx * Lyy - 0.81f * Lxy * Lxy;
    ;
    // Laplacian is given as Lxx + Lyy (matrix trace)
    float trace = Lxx + Lyy;

    hessian[mad24(gy, pitch, gx)] = det;
    laplacian[mad24(gy, pitch, gx)] = trace;
}

__attribute__((always_inline)) void readNeighbours(__global float* hessian, int x, int y, int pitch,
                                                   float N9[9])
{
    N9[0] = hessian[mad24(y - 1, pitch, x - 1)];
    N9[1] = hessian[mad24(y - 1, pitch, x)];
    N9[2] = hessian[mad24(y - 1, pitch, x + 1)];
    N9[3] = hessian[mad24(y, pitch, x - 1)];
    N9[4] = hessian[mad24(y, pitch, x)];
    N9[5] = hessian[mad24(y, pitch, x + 1)];
    N9[6] = hessian[mad24(y + 1, pitch, x - 1)];
    N9[7] = hessian[mad24(y + 1, pitch, x)];
    N9[8] = hessian[mad24(y + 1, pitch, x + 1)];
}

__attribute__((always_inline)) float polyPeak(float lower, float middle, float upper)
{
    float a = 0.5f * lower - middle + 0.5f * upper;
    float b = 0.5f * upper - 0.5f * lower;

    return -b / (2.0f * a);
}

__kernel void findScaleSpaceMaxima(int samples_x, int samples_y, __global float* hessian0,
                                   __global float* hessian1, __global float* hessian2,
                                   __global float* laplacian1, int pitch, float hessianThreshold,
                                   int sampleStep, int filterSize, int scaleDiff,
                                   counter_type keypointsCount,
                                   // Array of structures
                                   __global float* keypoints)

{
    int gx = get_global_id(0);
    int gy = get_global_id(1);

    if (gx >= samples_x + get_global_offset(0) || gy >= samples_y + get_global_offset(1))
    {
        return;
    }

    float v0 = hessian1[mad24(gy, pitch, gx)];
    if (v0 < hessianThreshold)
        return;

    // Get 3x3x3 neighbour of current pixel
    float N9[27];

    readNeighbours(hessian0, gx, gy, pitch, N9 + 0);
    readNeighbours(hessian1, gx, gy, pitch, N9 + 9);
    readNeighbours(hessian2, gx, gy, pitch, N9 + 18);

    // Check if it's a local maxima
    bool isMaxima = v0 > N9[0] && v0 > N9[1] && v0 > N9[2] && v0 > N9[3] && v0 > N9[4] &&
                    v0 > N9[5] && v0 > N9[6] && v0 > N9[7] && v0 > N9[8] &&

                    v0 > N9[9] && v0 > N9[10] && v0 > N9[11] && v0 > N9[12] && v0 > N9[14] &&
                    v0 > N9[15] && v0 > N9[16] && v0 > N9[17] &&

                    v0 > N9[18] && v0 > N9[19] && v0 > N9[20] && v0 > N9[21] && v0 > N9[22] &&
                    v0 > N9[23] && v0 > N9[24] && v0 > N9[25] && v0 > N9[26];
    if (!isMaxima)
        return;

    // Find maxima with subpixel accuracy using 2nd order polynomial fit
    float peakX = polyPeak(N9[12], v0, N9[14]);
    float peakY = polyPeak(N9[10], v0, N9[16]);
    float peakS = polyPeak(N9[4], v0, N9[22]);

    float interpX = (gx + peakX) * sampleStep;
    float interpY = (gy + peakY) * sampleStep;
    float interpS = filterSize + peakS * scaleDiff;

    // Add keypoints to a list (if only there's a place)
    int idx = atomic_inc(keypointsCount);
    if (idx < KEYPOINT_MAX)
    {
        // TODO: We could try to use sub buffers
        keypoints[KEYPOINT_MAX * KEYPOINT_X + idx] = interpX;
        keypoints[KEYPOINT_MAX * KEYPOINT_Y + idx] = interpY;
        keypoints[KEYPOINT_MAX * KEYPOINT_SCALE + idx] = BOXSIZE_TO_SCALE_COEFF * interpS;
        keypoints[KEYPOINT_MAX * KEYPOINT_RESPONSE + idx] = v0;

        int laplacian_sign = laplacian1[mad24(gy, pitch, gx)] >= 0 ? 1 : -1;
        keypoints[KEYPOINT_MAX * KEYPOINT_LAPLACIAN + idx] = as_float(laplacian_sign);
    }
}

bool haarInBounds(__read_only image2d_t integralImage, int x, int y, int grad_radius)
{
#if SYMETRIC_HAAR != 0
    if (x - grad_radius >= 0 && x + grad_radius + 1 < get_image_width(integralImage) &&
        y - grad_radius >= 0 && y + grad_radius + 1 < get_image_height(integralImage))
    {
        return true;
    }
    return false;
#else
    if (x - grad_radius >= 0 && x + grad_radius < get_image_width(integralImage) &&
        y - grad_radius >= 0 && y + grad_radius < get_image_height(integralImage))
    {
        return true;
    }
    return false;
#endif
}

// Tu podaje sie top-left obszaru oraz jego rozmiar
int boxFilterIntegral_(__read_only image2d_t integral, int x, int y, int w, int h)
{
    int A = read_imageui(integral, samp, (int2)(x, y)).x;
    int B = read_imageui(integral, samp, (int2)(x + w, y)).x;
    int C = read_imageui(integral, samp, (int2)(x, y + h)).x;
    int D = read_imageui(integral, samp, (int2)(x + w, y + h)).x;

    // is max() necessary here
    return max(0, A + D - B - C);
}

float2 calcHaarResponses(__read_only image2d_t integralImage, int x, int y, int grad_radius)
{
#if SYMETRIC_HAAR != 0
    int side = 2 * grad_radius + 1;
    float invarea = 1.0f / (side * grad_radius);
    float2 dxdy;
#if 0
    dxdy.x = 
        + boxFilterIntegral_(integralImage, x + 1          , y - grad_radius, grad_radius, side) * invarea 
        - boxFilterIntegral_(integralImage, x - grad_radius, y - grad_radius, grad_radius, side) * invarea;
    dxdy.y = 
        + boxFilterIntegral_(integralImage, x - grad_radius, y + 1          , side, grad_radius) * invarea
        - boxFilterIntegral_(integralImage, x - grad_radius, y - grad_radius, side, grad_radius) * invarea;
    return dxdy;
#else
    // Same as above but requires less VGPR on AMD 7000
    int A = read_imageui(integralImage, samp, (int2)(x - grad_radius, y - grad_radius)).x;
    int B = read_imageui(integralImage, samp, (int2)(x, y - grad_radius)).x;
    int C = read_imageui(integralImage, samp, (int2)(x + 1, y - grad_radius)).x;
    int D = read_imageui(integralImage, samp, (int2)(x + grad_radius + 1, y - grad_radius)).x;
    int E = read_imageui(integralImage, samp, (int2)(x - grad_radius, y + grad_radius + 1)).x;
    int F = read_imageui(integralImage, samp, (int2)(x, y + grad_radius + 1)).x;
    int G = read_imageui(integralImage, samp, (int2)(x + 1, y + grad_radius + 1)).x;
    int H = read_imageui(integralImage, samp, (int2)(x + grad_radius + 1, y + grad_radius + 1)).x;
    dxdy.x = (C + H - D - G) * invarea - (A + F - B - E) * invarea;

    int I = read_imageui(integralImage, samp, (int2)(x - grad_radius, y)).x;
    int J = read_imageui(integralImage, samp, (int2)(x + grad_radius + 1, y)).x;
    int K = read_imageui(integralImage, samp, (int2)(x - grad_radius, y + 1)).x;
    int L = read_imageui(integralImage, samp, (int2)(x + grad_radius + 1, y + 1)).x;
    dxdy.y = (K + H - L - E) * invarea - (A + J - D - I) * invarea;

    return dxdy;
#endif
#else
    // Optimised version - uses shared samples in Haar (6 samples instead of 8 in direct approach)
    float invarea = 1.0f / (2 * grad_radius * grad_radius);
    float2 dxdy;
    int A = read_imageui(integralImage, samp, (int2)(x - grad_radius, y - grad_radius)).x;
    int B = read_imageui(integralImage, samp, (int2)(x, y - grad_radius)).x;
    int E = read_imageui(integralImage, samp, (int2)(x + grad_radius, y - grad_radius)).x;
    int C = read_imageui(integralImage, samp, (int2)(x - grad_radius, y + grad_radius)).x;
    int D = read_imageui(integralImage, samp, (int2)(x, y + grad_radius)).x;
    int F = read_imageui(integralImage, samp, (int2)(x + grad_radius, y + grad_radius)).x;
    dxdy.x = -(A + D - B - C) * invarea + (B + F - E - D) * invarea;

    int G = read_imageui(integralImage, samp, (int2)(x - grad_radius, y)).x;
    int H = read_imageui(integralImage, samp, (int2)(x + grad_radius, y)).x;
    dxdy.y = +(G + F - H - C) * invarea - (A + H - E - G) * invarea;

    return dxdy;
#endif
}

__attribute__((always_inline)) float getAngle(float x, float y)
{
    float angle = atan2(y, x);
    if (angle < 0)
        angle += 2.0f * M_PI_F;
    return angle;
}

__kernel void uprightKeypointOrientation(__global float* keypoints, int numKeypoints)
{
    int gid = get_global_id(0);
    if (gid < numKeypoints)
        keypoints[KEYPOINT_MAX * KEYPOINT_ORIENTATION + gid] = 0.0f;
}

// These defines aren't here for easy tweak but rather for better readability
// Do NOT change it!
#define ORIENTATION_WINDOW_SIZE M_PI_F / 3.0f
#define ORIENTATION_WINDOW_STEP M_PI_F / 32.0f
#define ORIENTATION_WINDOW_NUM 64
#define ORIENTATION_SAMPLES 109
#define ORIENTATION_SAMPLES_PER_THREAD                                                             \
    (ORIENTATION_SAMPLES + ORIENTATION_WINDOW_NUM - 1) / ORIENTATION_WINDOW_NUM

__attribute__((reqd_work_group_size(ORIENTATION_WINDOW_NUM, 1, 1))) __kernel void
    findKeypointOrientation(__read_only image2d_t integralImage, __global float* keypoints,
                            __constant int2* samplesCoords, __constant float* gaussianWeights)
{
    // One work group computes orientation for one feature
    int bid = get_group_id(0);
    int tid = get_local_id(0);

    // Load in keypoint parameters
    // Tried loading using shared memory but in the end this is faster (at least on AMD)
    float featureX = keypoints[KEYPOINT_MAX * KEYPOINT_X + bid];
    float featureY = keypoints[KEYPOINT_MAX * KEYPOINT_Y + bid];
    float featureScale = keypoints[KEYPOINT_MAX * KEYPOINT_SCALE + bid];

    __local float smem_dx[ORIENTATION_SAMPLES];
    __local float smem_dy[ORIENTATION_SAMPLES];
    __local float smem_angle[ORIENTATION_SAMPLES];

#pragma unroll
    for (int ii = 0; ii < ORIENTATION_SAMPLES_PER_THREAD; ++ii)
    {
        int i = ii * ORIENTATION_WINDOW_NUM + tid;
        if (i < ORIENTATION_SAMPLES)
        {
            int x = convert_int(round(featureX + samplesCoords[i].x * featureScale));
            int y = convert_int(round(featureY + samplesCoords[i].y * featureScale));
            int grad_radius = convert_int(round(2 * featureScale));

            // If there's no full neighbourhood for this gradient - set it to 0 response
            if (haarInBounds(integralImage, x, y, grad_radius))
            {
                float2 dxdy = calcHaarResponses(integralImage, x, y, grad_radius);
                smem_dx[i] = dxdy.x * gaussianWeights[i];
                smem_dy[i] = dxdy.y * gaussianWeights[i];
                smem_angle[i] = getAngle(dxdy.x, dxdy.y);
            }
            else
            {
                // this should stop this dx, dy from adding to any window
                smem_angle[i] = 999.0f;
            }
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // Each thread computes one sliding window (64 in total)
    // This way we got fast window calculation with broadcasting
    float windowBegin = ORIENTATION_WINDOW_STEP * tid;
    float sumX = 0.0f, sumY = 0.0f;

#pragma unroll 16
    for (int i = 0; i < ORIENTATION_SAMPLES; ++i)
    {
        float d = smem_angle[i] - windowBegin;
        if (d > 0 && d < ORIENTATION_WINDOW_SIZE)
        {
            sumX += smem_dx[i];
            sumY += smem_dy[i];
        }
    }

    // Let the threads share their hackwork
    volatile __local float smem_sum[ORIENTATION_WINDOW_NUM];
    volatile __local float smem_sumX[ORIENTATION_WINDOW_NUM];
    volatile __local float smem_sumY[ORIENTATION_WINDOW_NUM];

    smem_sum[tid] = sumX * sumX + sumY * sumY;
    smem_sumX[tid] = sumX;
    smem_sumY[tid] = sumY;

    barrier(CLK_LOCAL_MEM_FENCE);

    // No need for barrier thanks to proper work group size - group of 32 threads in first reduction
    // phase (half of workgroup is idle) are working - conveniently it's one warp (one half of
    // wavefront) in total. They are executed in lock-steps which makes barrier redundant. Only if
    // we would use work group of more than 64 (or 128 for AMD) we would need to add barrier() after
    // every reduction step (like in CPU case)
#define REDUCE(num)                                                                                \
    {                                                                                              \
        float v = smem_sum[tid + num];                                                             \
        if (smem_sum[tid] < v)                                                                     \
        {                                                                                          \
            smem_sumX[tid] = smem_sumX[tid + num];                                                 \
            smem_sumY[tid] = smem_sumY[tid + num];                                                 \
            smem_sum[tid] = v;                                                                     \
        }                                                                                          \
    }
#if WARP_SIZE == 32 || WARP_SIZE == 64
    if (tid < 32)
    {
        if (ORIENTATION_WINDOW_NUM >= 64)
        {
            REDUCE(32);
        }
        if (ORIENTATION_WINDOW_NUM >= 32)
        {
            REDUCE(16);
        }
        if (ORIENTATION_WINDOW_NUM >= 16)
        {
            REDUCE(8);
        }
        if (ORIENTATION_WINDOW_NUM >= 8)
        {
            REDUCE(4);
        }
        if (ORIENTATION_WINDOW_NUM >= 4)
        {
            REDUCE(2);
        }
        if (ORIENTATION_WINDOW_NUM >= 2)
        {
            REDUCE(1);
        }
    }
#else
// For CPUs this should be used (we can't exploit wavefront/warp size)
// In fact - on AMD if there's a kernel attribute reqd_work_group_size equal to 64
// work items all barrier calls are removed from resulting ISA code.
#pragma unroll 6
    for (int s = ORIENTATION_WINDOW_NUM / 2; s > 0; s >>= 1)
    {
        if (tid < s)
            REDUCE(s);
        barrier(CLK_LOCAL_MEM_FENCE);
    }
#endif

    // First thread writes computed orientation
    if (tid == 0)
    {
        float angle = getAngle(smem_sumX[0], smem_sumY[0]);
        keypoints[KEYPOINT_MAX * KEYPOINT_ORIENTATION + bid] = angle;
    }
}

#define ORIENTATION_SMEM_SIZE 128

// This kernel uses common local memory area which results in lesser LDS usage (from 2kB to 1.5kB)
// Consequently this kernel achieves better occupancy but it doesn't mean you get faster processing
// (something can be botleneck)
__attribute__((reqd_work_group_size(ORIENTATION_WINDOW_NUM, 1, 1))) __kernel void
    findKeypointOrientation2(__read_only image2d_t integralImage, __global float* keypoints,
                             __constant int2* samplesCoords, __constant float* gaussianWeights)
{
    // One work group computes orientation for one feature
    int bid = get_group_id(0);
    int tid = get_local_id(0);

    // Load in keypoint parameters
    // Tried loading using shared memory but in the end this is faster (at least on AMD)
    float featureX = keypoints[KEYPOINT_MAX * KEYPOINT_X + bid];
    float featureY = keypoints[KEYPOINT_MAX * KEYPOINT_Y + bid];
    float featureScale = keypoints[KEYPOINT_MAX * KEYPOINT_SCALE + bid];

    __local float smem[3 * ORIENTATION_SMEM_SIZE];

    __local float* smem_dx = smem + ORIENTATION_SMEM_SIZE * 0;
    __local float* smem_dy = smem + ORIENTATION_SMEM_SIZE * 1;
    __local float* smem_angle = smem + ORIENTATION_SMEM_SIZE * 2;

#pragma unroll
    for (int ii = 0; ii < ORIENTATION_SAMPLES_PER_THREAD; ++ii)
    {
        int i = ii * ORIENTATION_WINDOW_NUM + tid;
        if (i < ORIENTATION_SAMPLES)
        {
            int x = convert_int(round(featureX + samplesCoords[i].x * featureScale));
            int y = convert_int(round(featureY + samplesCoords[i].y * featureScale));
            int grad_radius = convert_int(round(2 * featureScale));

            // If there's no full neighbourhood for this gradient - set it to 0 response
            if (haarInBounds(integralImage, x, y, grad_radius))
            {
                float2 dxdy = calcHaarResponses(integralImage, x, y, grad_radius);
                smem_dx[i] = dxdy.x * gaussianWeights[i];
                smem_dy[i] = dxdy.y * gaussianWeights[i];
                smem_angle[i] = getAngle(dxdy.x, dxdy.y);
            }
            else
            {
                // this should stop this dx, dy from adding to any window
                smem_angle[i] = 999.0f;
            }
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // Each thread computes one sliding window (64 in total)
    // This way we got fast window calculation with broadcasting
    float windowBegin = ORIENTATION_WINDOW_STEP * tid;
    float sumX = 0.0f, sumY = 0.0f;

#pragma unroll 16
    for (int i = 0; i < ORIENTATION_SAMPLES; ++i)
    {
        float d = smem_angle[i] - windowBegin;
        if (d > 0 && d < ORIENTATION_WINDOW_SIZE)
        {
            sumX += smem_dx[i];
            sumY += smem_dy[i];
        }
    }

    // Let the threads share their hackwork
    volatile __local float* smem_sum = smem + ORIENTATION_WINDOW_NUM * 0;
    volatile __local float* smem_sumX = smem + ORIENTATION_WINDOW_NUM * 1;
    volatile __local float* smem_sumY = smem + ORIENTATION_WINDOW_NUM * 2;

    smem_sum[tid] = sumX * sumX + sumY * sumY;
    smem_sumX[tid] = sumX;
    smem_sumY[tid] = sumY;

    barrier(CLK_LOCAL_MEM_FENCE);

    // No need for barrier thanks to proper work group size - group of 32 threads in first reduction
    // phase (half of workgroup is idle) are working - conveniently it's one warp (one half of
    // wavefront) in total. They are executed in lock-steps which makes barrier redundant. Only if
    // we would use work group of more than 64 (or 128 for AMD) we would need to add barrier() after
    // every reduction step (like in CPU case)
#define REDUCE(num)                                                                                \
    {                                                                                              \
        float v = smem_sum[tid + num];                                                             \
        if (smem_sum[tid] < v)                                                                     \
        {                                                                                          \
            smem_sumX[tid] = smem_sumX[tid + num];                                                 \
            smem_sumY[tid] = smem_sumY[tid + num];                                                 \
            smem_sum[tid] = v;                                                                     \
        }                                                                                          \
    }
#if WARP_SIZE == 32 || WARP_SIZE == 64
    if (tid < 32)
    {
        if (ORIENTATION_WINDOW_NUM >= 64)
        {
            REDUCE(32);
        }
        if (ORIENTATION_WINDOW_NUM >= 32)
        {
            REDUCE(16);
        }
        if (ORIENTATION_WINDOW_NUM >= 16)
        {
            REDUCE(8);
        }
        if (ORIENTATION_WINDOW_NUM >= 8)
        {
            REDUCE(4);
        }
        if (ORIENTATION_WINDOW_NUM >= 4)
        {
            REDUCE(2);
        }
        if (ORIENTATION_WINDOW_NUM >= 2)
        {
            REDUCE(1);
        }
    }
#else
// For CPUs this should be used (we can't exploit wavefront/warp size)
// In fact - on AMD if there's a kernel attribute reqd_work_group_size equal to 64
// work items all barrier calls are removed from resulting ISA code.
#pragma unroll 6
    for (int s = ORIENTATION_WINDOW_NUM / 2; s > 0; s >>= 1)
    {
        if (tid < s)
            REDUCE(s);
        barrier(CLK_LOCAL_MEM_FENCE);
    }
#endif

    // First thread writes computed orientation
    if (tid == 0)
    {
        float angle = getAngle(smem_sumX[0], smem_sumY[0]);
        keypoints[KEYPOINT_MAX * KEYPOINT_ORIENTATION + bid] = angle;
    }
}

__attribute__((always_inline)) float Gaussian(float x, float y, float sigma)
{
    float sigma2 = 2.0f * sigma * sigma;
    return (1.0f / (M_PI_F * sigma2)) * exp(-(x * x + y * y) / (sigma2));
}

__attribute__((reqd_work_group_size(5, 5, 1))) __kernel void
    calculateDescriptors(__read_only image2d_t integralImage, __global float* keypoints,
                         __global float* descriptors)
{
    int keypointId = get_group_id(0);
    int regionId = get_group_id(1);
    int tid = get_local_id(0) + get_local_id(1) * 5;

    // Load in keypoint parameters
    // Tried loading using shared memory but in the end this is faster (at least on AMD)
    float featureX = keypoints[KEYPOINT_MAX * KEYPOINT_X + keypointId];
    float featureY = keypoints[KEYPOINT_MAX * KEYPOINT_Y + keypointId];
    float featureScale = keypoints[KEYPOINT_MAX * KEYPOINT_SCALE + keypointId];
    float featureOrientation = keypoints[KEYPOINT_MAX * KEYPOINT_ORIENTATION + keypointId];

    float c, s;
    s = sincos(featureOrientation, &c);

    // WARNING: Many magic numbers ahead
    // x & 3 := x % 4 but AND is faster
    int regionX = ((regionId & 3) - 2) * 5; // regionX = [-10,-5,0,5]
    int regionY = ((regionId / 4) - 2) * 5; // regionY = [-10,-5,0,5]

    int subRegionX = regionX + get_local_id(0);
    int subRegionY = regionY + get_local_id(1);

    int sample_x =
        (int)round(featureX + (subRegionX * featureScale * c - subRegionY * featureScale * s));
    int sample_y =
        (int)round(featureY + (subRegionX * featureScale * s + subRegionY * featureScale * c));
    int grad_radius = (int)round(featureScale);

    volatile __local float smem_sumDx[25];
    volatile __local float smem_sumDy[25];
    volatile __local float smem_sumADx[25];
    volatile __local float smem_sumADy[25];

    __private float dx = 0.0f, dy = 0.0f;

    // If there's no full neighbourhood for this gradient - set it to 0 response
    if (haarInBounds(integralImage, sample_x, sample_y, grad_radius))
    {
        float2 dxdy = calcHaarResponses(integralImage, sample_x, sample_y, grad_radius);
        float weight = Gaussian(subRegionX, subRegionY, BASIC_DESC_SIGMA * featureScale);
        dxdy.x *= weight;
        dxdy.y *= weight;

        dx = c * dxdy.x + s * dxdy.y;
        dy = -s * dxdy.x + c * dxdy.y;
    }

    smem_sumDx[tid] = dx;
    smem_sumDy[tid] = dy;
    smem_sumADx[tid] = fabs(dx);
    smem_sumADy[tid] = fabs(dy);

    barrier(CLK_LOCAL_MEM_FENCE);

    // Reduction
    if (tid < 9)
    {
        smem_sumDx[tid] += smem_sumDx[tid + 16];
        smem_sumDy[tid] += smem_sumDy[tid + 16];
        smem_sumADx[tid] += smem_sumADx[tid + 16];
        smem_sumADy[tid] += smem_sumADy[tid + 16];
    }

    // NVIDIA and AMD can do it without barriers
#if WARP_SIZE == 32 || WARP_SIZE == 64
    if (tid < 8)
    {
        smem_sumDx[tid] += smem_sumDx[tid + 8];
        smem_sumDy[tid] += smem_sumDy[tid + 8];
        smem_sumADx[tid] += smem_sumADx[tid + 8];
        smem_sumADy[tid] += smem_sumADy[tid + 8];

        smem_sumDx[tid] += smem_sumDx[tid + 4];
        smem_sumDy[tid] += smem_sumDy[tid + 4];
        smem_sumADx[tid] += smem_sumADx[tid + 4];
        smem_sumADy[tid] += smem_sumADy[tid + 4];

        smem_sumDx[tid] += smem_sumDx[tid + 2];
        smem_sumDy[tid] += smem_sumDy[tid + 2];
        smem_sumADx[tid] += smem_sumADx[tid + 2];
        smem_sumADy[tid] += smem_sumADy[tid + 2];

        smem_sumDx[tid] += smem_sumDx[tid + 1];
        smem_sumDy[tid] += smem_sumDy[tid + 1];
        smem_sumADx[tid] += smem_sumADx[tid + 1];
        smem_sumADy[tid] += smem_sumADy[tid + 1];
    }
#else
    barrier(CLK_LOCAL_MEM_FENCE);

#pragma unroll 4
    for (int s = 8; s > 0; s >>= 1)
    {
        if (tid < s)
        {
            smem_sumDx[tid] += smem_sumDx[tid + s];
            smem_sumDy[tid] += smem_sumDy[tid + s];
            smem_sumADx[tid] += smem_sumADx[tid + s];
            smem_sumADy[tid] += smem_sumADy[tid + s];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
#endif

    if (tid == 0)
    {
        descriptors[keypointId * 64 + 4 * regionId + 0] = smem_sumDx[0];
        descriptors[keypointId * 64 + 4 * regionId + 1] = smem_sumDy[0];
        descriptors[keypointId * 64 + 4 * regionId + 2] = smem_sumADx[0];
        descriptors[keypointId * 64 + 4 * regionId + 3] = smem_sumADy[0];
    }
}

__attribute__((reqd_work_group_size(9, 9, 1))) __kernel void
    calculateDescriptorsMSURF(__read_only image2d_t integralImage, __global float* keypoints,
                              __global float* descriptors)
{
    int keypointId = get_group_id(0);
    int regionId = get_group_id(1);
    int tid = get_local_id(0) + get_local_id(1) * 9;

    // Load in keypoint parameters
    // Tried loading using shared memory but in the end this is faster (at least on AMD)
    float featureX = keypoints[KEYPOINT_MAX * KEYPOINT_X + keypointId];
    float featureY = keypoints[KEYPOINT_MAX * KEYPOINT_Y + keypointId];
    float featureScale = keypoints[KEYPOINT_MAX * KEYPOINT_SCALE + keypointId];
    float featureOrientation = keypoints[KEYPOINT_MAX * KEYPOINT_ORIENTATION + keypointId];

    float c, s;
    s = sincos(featureOrientation, &c);

    // WARNING: Many magic numbers ahead
    // x & 3 := x % 4 but AND is faster
    int regionX = ((regionId & 3) - 2) * 5; // regionX = [-10,-5,0,5]
    int regionY = ((regionId / 4) - 2) * 5; // regionY = [-10,-5,0,5]

    // regionX + {-2,..6}
    int subRegionX = regionX + (get_local_id(0) - 2);
    int subRegionY = regionY + (get_local_id(1) - 2);

    int sample_x =
        (int)round(featureX + (subRegionX * featureScale * c - subRegionY * featureScale * s));
    int sample_y =
        (int)round(featureY + (subRegionX * featureScale * s + subRegionY * featureScale * c));
    int grad_radius = (int)round(featureScale);

    volatile __local float smem_sumDx[81];
    volatile __local float smem_sumDy[81];
    volatile __local float smem_sumADx[81];
    volatile __local float smem_sumADy[81];

    __private float dx = 0.0f, dy = 0.0f;

    // If there's no full neighbourhood for this gradient - set it to 0 response
    if (haarInBounds(integralImage, sample_x, sample_y, grad_radius))
    {
        float2 dxdy = calcHaarResponses(integralImage, sample_x, sample_y, grad_radius);
        float weight = Gaussian(get_local_id(0) - 4, get_local_id(1) - 4, DESC_SUBREGION_SIGMA);
        dxdy.x *= weight;
        dxdy.y *= weight;

        dx = c * dxdy.x + s * dxdy.y;
        dy = -s * dxdy.x + c * dxdy.y;
    }

    smem_sumDx[tid] = dx;
    smem_sumDy[tid] = dy;
    smem_sumADx[tid] = fabs(dx);
    smem_sumADy[tid] = fabs(dy);

    barrier(CLK_LOCAL_MEM_FENCE);

    // Reduction
    if (tid < 17)
    {
        smem_sumDx[tid] += smem_sumDx[tid + 64];
        smem_sumDy[tid] += smem_sumDy[tid + 64];
        smem_sumADx[tid] += smem_sumADx[tid + 64];
        smem_sumADy[tid] += smem_sumADy[tid + 64];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

#pragma unroll 6
    for (int s = 32; s > 0; s >>= 1)
    {
        if (tid < s)
        {
            smem_sumDx[tid] += smem_sumDx[tid + s];
            smem_sumDy[tid] += smem_sumDy[tid + s];
            smem_sumADx[tid] += smem_sumADx[tid + s];
            smem_sumADy[tid] += smem_sumADy[tid + s];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (tid == 0)
    {
        float cx = regionX * 0.2f + 0.5f;
        float cy = regionY * 0.2f + 0.5f;
        float weight = Gaussian(cx, cy, DESC_REGION_SIGMA);

        descriptors[keypointId * 64 + 4 * regionId + 0] = weight * smem_sumDx[0];
        descriptors[keypointId * 64 + 4 * regionId + 1] = weight * smem_sumDy[0];
        descriptors[keypointId * 64 + 4 * regionId + 2] = weight * smem_sumADx[0];
        descriptors[keypointId * 64 + 4 * regionId + 3] = weight * smem_sumADy[0];
    }
}

__attribute__((reqd_work_group_size(64, 1, 1))) __kernel void
    normalizeDescriptors(__global float* descriptors)
{
    // One work group normalize one descriptor
    int bid = get_group_id(0);
    int tid = get_local_id(0);

    // Read unnormalized descriptor 'tid'-th value
    __private float p = descriptors[bid * 64 + tid];

    // Cache the square of p in shared memory
    volatile __local float smem[64];
    smem[tid] = p * p;
    barrier(CLK_LOCAL_MEM_FENCE);

#if WARP_SIZE == 32 || WARP_SIZE == 64
    // Reduce it (sum)
    if (tid < 32)
    {
        smem[tid] += smem[tid + 32];
        smem[tid] += smem[tid + 16];
        smem[tid] += smem[tid + 8];
        smem[tid] += smem[tid + 4];
        smem[tid] += smem[tid + 2];
        smem[tid] += smem[tid + 1];
    }
    barrier(CLK_LOCAL_MEM_FENCE);
#else
#pragma unroll 6
    for (int s = 32; s > 0; s >>= 1)
    {
        if (tid < s)
            smem[tid] += smem[tid + s];
        barrier(CLK_LOCAL_MEM_FENCE);
    }
#endif

    // Calculate sqrt(sum(descriptor_i))
    __local float invLenSqrt;
    if (tid == 0)
        invLenSqrt = 1.0f / sqrt(smem[0]);
    barrier(CLK_LOCAL_MEM_FENCE);

    // Write back normalized descriptor value at tid position
    descriptors[bid * 64 + tid] = p * invLenSqrt;
}