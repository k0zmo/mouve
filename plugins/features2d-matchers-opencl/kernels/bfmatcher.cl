/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#if __OPENCL_VERSION__ >= CL_VERSION_1_2 && CL_LANGUAGE_CPP == 1

#if defined(cl_ext_atomic_counters_32)
#pragma OPENCL EXTENSION cl_ext_atomic_counters_32 : enable
#define counter_type counter32_t
#else
#define counter_type volatile __global int*
#endif

struct L2Dist
{
    typedef float value_type;

    L2Dist() : _sum(0.0f) {}

    __attribute__((always_inline)) void reduceIter(float v1, float v2)
    {
        float q = v1 - v2;
        _sum += q * q;
    }

    __attribute__((always_inline)) operator float() const { return _sum; }

    __attribute__((always_inline)) void finish() { _sum = native_sqrt(_sum); }

    float _sum;
};

struct HammingDist
{
    typedef int value_type;

    HammingDist() : _sum(0) {}

    __attribute__((always_inline)) void reduceIter(int v1, int v2) { _sum += popcount(v1 ^ v2); }

    __attribute__((always_inline)) operator int() const { return _sum; }

    __attribute__((always_inline)) void finish() { ; }

    int _sum;
};

template <int BLOCK_SIZE>
void findBestMatch(float& bestDistance1, float& bestDistance2, int& bestTrainIdx1,
                   __local float* s_distance, __local int* s_trainIdx, int lx, int ly)
{
    float myBestDistance1 = MAXFLOAT;
    float myBestDistance2 = MAXFLOAT;
    int myBestTrainIdx1 = -1;

    s_distance += ly * BLOCK_SIZE;
    s_trainIdx += ly * BLOCK_SIZE;

    s_distance[lx] = bestDistance1;
    s_trainIdx[lx] = bestTrainIdx1;

    barrier(CLK_LOCAL_MEM_FENCE);

    if (lx == 0)
    {
#pragma unroll
        for (int i = 0; i < BLOCK_SIZE; ++i)
        {
            float val = s_distance[i];

            if (val < myBestDistance1)
            {
                myBestDistance2 = myBestDistance1;

                myBestDistance1 = val;
                myBestTrainIdx1 = s_trainIdx[i];
            }
            else if (val < myBestDistance2)
            {
                myBestDistance2 = val;
            }
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    s_distance[lx] = bestDistance2;

    barrier(CLK_LOCAL_MEM_FENCE);

    if (lx == 0)
    {
#pragma unroll
        for (int i = 0; i < BLOCK_SIZE; ++i)
        {
            float val = s_distance[i];

            if (val < myBestDistance2)
            {
                myBestDistance2 = val;
            }
        }
    }

    bestDistance1 = myBestDistance1;
    bestDistance2 = myBestDistance2;
    bestTrainIdx1 = myBestTrainIdx1;
}

typedef struct DMatch
{
    int queryIdx;
    int trainIdx;
    float dist;
} DMatch_t;

template <class T, class Dist, int BLOCK_SIZE, int DESC_LEN>
__kernel void bruteForceMatch_nndrMatch(__global T* query, __global T* train,
                                        __global DMatch_t* matches, counter_type matchesCount,
                                        __local int* smem, int queryRows, int trainRows,
                                        float distanceRatio)
{
    const int lx = get_local_id(0);
    const int ly = get_local_id(1);
    const int groupId = get_group_id(0);

    const int queryIdx = groupId * BLOCK_SIZE + ly;
    __local typename Dist::value_type* s_query = (__local typename Dist::value_type*)smem;
    __local typename Dist::value_type* s_train =
        (__local typename Dist::value_type*)smem + BLOCK_SIZE * DESC_LEN;

#pragma unroll
    for (int i = 0; i < DESC_LEN / BLOCK_SIZE; ++i)
    {
        const int loadX = i * BLOCK_SIZE + lx;
        s_query[ly * DESC_LEN + loadX] =
            loadX < DESC_LEN ? query[min(queryIdx, queryRows - 1) * DESC_LEN + loadX] : 0;
    }

    float myBestDistance1 = MAXFLOAT;
    float myBestDistance2 = MAXFLOAT;
    int myBestTrainIdx1 = -1;

    for (int t = 0, endt = (trainRows + BLOCK_SIZE - 1) / BLOCK_SIZE; t < endt; ++t)
    {
        Dist dist;

#pragma unroll
        for (int i = 0; i < DESC_LEN / BLOCK_SIZE; ++i)
        {
            const int loadX = i * BLOCK_SIZE + lx;

            s_train[lx * BLOCK_SIZE + ly] =
                loadX < DESC_LEN ? train[min(t * BLOCK_SIZE + ly, trainRows - 1) * DESC_LEN + loadX]
                                 : 0;

            barrier(CLK_LOCAL_MEM_FENCE);

            // Causes register spilling on AMD
            //#pragma unroll
            for (int j = 0; j < BLOCK_SIZE; ++j)
                dist.reduceIter(s_query[ly * DESC_LEN + i * BLOCK_SIZE + j],
                                s_train[j * BLOCK_SIZE + lx]);

            barrier(CLK_LOCAL_MEM_FENCE);
        }

        dist.finish();

        const int trainIdx = t * BLOCK_SIZE + lx;

        if (queryIdx < queryRows && trainIdx < trainRows)
        {
            if (dist < myBestDistance1)
            {
                myBestDistance2 = myBestDistance1;
                myBestDistance1 = dist;
                myBestTrainIdx1 = trainIdx;
            }
            else if (dist < myBestDistance2)
            {
                myBestDistance2 = dist;
            }
        }
    }

    __local float* s_distance = (__local float*)smem;
    __local int* s_trainIdx = (__local int*)smem + BLOCK_SIZE * BLOCK_SIZE;

    findBestMatch<BLOCK_SIZE>(myBestDistance1, myBestDistance2, myBestTrainIdx1, s_distance,
                              s_trainIdx, lx, ly);

    if (queryIdx < queryRows && lx == 0)
    {
        if (myBestDistance1 <= distanceRatio * myBestDistance2)
        {
            int idx = atomic_inc(matchesCount);
            // NOTE: We know for sure matches can't be more than queryRows
            matches[idx].queryIdx = queryIdx;
            matches[idx].trainIdx = myBestTrainIdx1;
            matches[idx].dist = myBestDistance1;
        }
    }
}

template __attribute__((mangled_name(bruteForceMatch_nndrMatch_SURF))) __kernel void
    bruteForceMatch_nndrMatch<float, L2Dist, 16, 64>(__global float* query, __global float* train,
                                                     __global DMatch_t* matches,
                                                     counter_type matchesCount, __local int* smem,
                                                     int queryRows, int trainRows,
                                                     float distanceRatio);

template __attribute__((mangled_name(bruteForceMatch_nndrMatch_SIFT))) __kernel void
    bruteForceMatch_nndrMatch<float, L2Dist, 16, 128>(__global float* query, __global float* train,
                                                      __global DMatch_t* matches,
                                                      counter_type matchesCount, __local int* smem,
                                                      int queryRows, int trainRows,
                                                      float distanceRatio);

template __attribute__((mangled_name(bruteForceMatch_nndrMatch_FREAK))) __kernel void
    bruteForceMatch_nndrMatch<uchar, HammingDist, 16, 64>(__global uchar* query,
                                                          __global uchar* train,
                                                          __global DMatch_t* matches,
                                                          counter_type matchesCount,
                                                          __local int* smem, int queryRows,
                                                          int trainRows, float distanceRatio);

template __attribute__((mangled_name(bruteForceMatch_nndrMatch_ORB))) __kernel void
    bruteForceMatch_nndrMatch<uchar, HammingDist, 16, 32>(__global uchar* query,
                                                          __global uchar* train,
                                                          __global DMatch_t* matches,
                                                          counter_type matchesCount,
                                                          __local int* smem, int queryRows,
                                                          int trainRows, float distanceRatio);
#endif
