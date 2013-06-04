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

// This needs to be defined:
// KERNEL_NAME (i.e. bruteForceMatch_2nnMatch_SURF)
// BLOCK_SIZE (i.e. 16)
// DESC_LEN (i.e. 64)
// QUERY_TYPE (i.e. float)
// DIST_TYPE (i.e. float)
// DIST_FUNCTION (i.e. l2DistIter)

#if __OPENCL_VERSION__ < CL_VERSION_1_2

int popcount(int i)
{
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

#endif

__attribute__((always_inline)) float hammingDistIter(int x1, int x2) { return (float) popcount(x1 ^ x2); }
__attribute__((always_inline)) float l2DistIter(float x1, float x2) { float q = x1 - x2; return q * q; }

__kernel void
KERNEL_NAME(__global QUERY_TYPE* query,
            __global QUERY_TYPE* train,
            __global int2* bestTrainIdx,
            __global float2* bestDistance,
            __local int* smem,
            int queryRows,
            int trainRows) 
{
    const int lx = get_local_id(0);
    const int ly = get_local_id(1);
    const int groupId = get_group_id(0);

    const int queryIdx = groupId*BLOCK_SIZE + ly;
    __local DIST_TYPE* s_query = (__local DIST_TYPE*) smem;
    __local DIST_TYPE* s_train = (__local DIST_TYPE*) smem + BLOCK_SIZE*DESC_LEN;
    
    #pragma unroll
    for(int i = 0; i < DESC_LEN / BLOCK_SIZE; ++i)
    {
        const int loadX = i*BLOCK_SIZE + lx;
        s_query[ly*DESC_LEN + loadX] = loadX < DESC_LEN
            ? query[min(queryIdx, queryRows-1)*DESC_LEN + loadX]
            : 0;
    }

    float myBestDistance1 = MAXFLOAT;
    float myBestDistance2 = MAXFLOAT;
    int myBestTrainIdx1 = -1;
    int myBestTrainIdx2 = -1;

    for(int t = 0, endt = (trainRows + BLOCK_SIZE - 1) / BLOCK_SIZE; t < endt; ++t)
    {
        float dist = 0.0;

        //#pragma unroll
        for(int i = 0; i < DESC_LEN / BLOCK_SIZE; ++i)
        {
            const int loadX = i*BLOCK_SIZE + lx;

            s_train[lx*BLOCK_SIZE + ly] = loadX < DESC_LEN 
                ? train[min(t*BLOCK_SIZE + ly, trainRows-1) * DESC_LEN + loadX]
                : 0;

            barrier(CLK_LOCAL_MEM_FENCE);

            for (int j = 0 ; j < BLOCK_SIZE ; j++)
                dist += DIST_FUNCTION(s_query[ly*DESC_LEN + i*BLOCK_SIZE + j], s_train[j*BLOCK_SIZE + lx]);

            barrier(CLK_LOCAL_MEM_FENCE);
        }

        const int trainIdx = t*BLOCK_SIZE + lx;

        if(queryIdx < queryRows && trainIdx < trainRows)
        {
            if(dist < myBestDistance1)
            {
                myBestDistance2 = myBestDistance1;
                myBestTrainIdx2 = myBestTrainIdx1;
                myBestDistance1 = dist;
                myBestTrainIdx1 = trainIdx;
            }
            else if(dist < myBestDistance2)
            {
                myBestDistance2 = dist;
                myBestTrainIdx2 = trainIdx;
            }
        }
    }

    __local float* s_distance = (__local float*) smem;
    __local int* s_trainIdx = (__local int*) smem + BLOCK_SIZE*BLOCK_SIZE;

    float _myBestDistance1 = MAXFLOAT;
    float _myBestDistance2 = MAXFLOAT;
    int _myBestTrainIdx1 = -1;
    int _myBestTrainIdx2 = -1;

    s_distance += ly*BLOCK_SIZE;
    s_trainIdx += ly*BLOCK_SIZE;

    s_distance[lx] = myBestDistance1;
    s_trainIdx[lx] = myBestTrainIdx1;

    barrier(CLK_LOCAL_MEM_FENCE);

    if(lx == 0)
    {
        #pragma unroll
        for(int i = 0; i < BLOCK_SIZE; ++i)
        {
            float val = s_distance[i];

            if(val < _myBestDistance1)
            {
                _myBestDistance2 = _myBestDistance1;
                _myBestTrainIdx2 = _myBestTrainIdx1;

                _myBestDistance1 = val;
                _myBestTrainIdx1 = s_trainIdx[i];
            }
            else if(val < _myBestDistance2)
            {
                _myBestDistance2 = val;
                _myBestTrainIdx2 = s_trainIdx[i];
            }
        }
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    s_distance[lx] = myBestDistance2;
    s_trainIdx[lx] = myBestTrainIdx2;

    barrier(CLK_LOCAL_MEM_FENCE);

    if(lx == 0)
    {
        #pragma unroll
        for(int i = 0; i < BLOCK_SIZE; ++i)
        {
            float val = s_distance[i];

            if(val < _myBestDistance2)
            {
                _myBestDistance2 = val;
                _myBestTrainIdx2 = s_trainIdx[i];
            }
        }
    }

    if(queryIdx < queryRows && lx == 0)
    {
        bestTrainIdx[queryIdx] = (int2)(_myBestTrainIdx1, _myBestTrainIdx2);
        bestDistance[queryIdx] = (float2)(_myBestDistance1, _myBestDistance2);
    }
}