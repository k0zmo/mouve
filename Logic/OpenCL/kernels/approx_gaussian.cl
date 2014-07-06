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

// These OpenCL kernels are based on the work "Constant Time Gaussian Blur"
// by Lin Nan <calin@nvidia.com>, NVIDIA, 2011

// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

// Sample defines:
//  WARP_SIZE=64
//  WARP_SIZE_LOG_2=6
//  NUM_THREADS=256
//  NUM_WARPS=4
//  IMAGE_WIDTH=512
//  IMAGE_HEIGHT=512
//  TEXELS_PER_THREAD=2
//  APPROX_GAUSSIAN_PASS=1

#if APPROX_GAUSSIAN_PASS == 0
#  define approxGaussian_pass_image approxGaussian_horiz_image
#  define IMAGE_SIZE IMAGE_WIDTH
#  define IMAGE_COORDS(x, y) (int2)(x, y)
#else
#  define approxGaussian_pass_image approxGaussian_vert_image
#  define IMAGE_SIZE IMAGE_HEIGHT
#  define IMAGE_COORDS(x, y) (int2)(y, x)
#endif

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

__attribute__((always_inline))
float boxFilter1D(uint location,
                  __local float* smem,
                  // Filtering parameters
                  float halfBoxFilterWidth,
                  float fracHalfBoxFilterWidth,
                  float rcpBoxFilterWidth)
{
    // Calculate the sampling locations (left and right side of the filter window).
    // We treat the original data as a piecewise box function, thus the sum is a
    // piecewise linear function. For arbitrary FP sampling location, we interpolate
    // the value from adjacent data pairs.

    float center = location - 0.5f;
    int L_a = clamp((int)ceil(center - halfBoxFilterWidth), 0, IMAGE_SIZE - 1);
    int L_b = clamp(L_a - 1, 0, IMAGE_SIZE - 1);
    int R_a = clamp((int)floor(center + halfBoxFilterWidth), 0, IMAGE_SIZE - 1);
    int R_b = clamp(R_a + 1, 0, IMAGE_SIZE - 1);
    
    float L_value = mix(smem[L_a], smem[L_b], fracHalfBoxFilterWidth);
    float R_value = mix(smem[R_a], smem[R_b], fracHalfBoxFilterWidth);

    return (R_value - L_value) * rcpBoxFilterWidth;
}

__attribute__((always_inline)) 
float scanWarpInclusive(uint tid, float value, uint size, __local volatile float* smem)
{
    // Naive inclusive scan: O(N * log2(N)) operations within a warp (32 threads)
    // All threads within a warp always execute the same instruction, thus we don't
    // need to do sync-up.
    uint location = 2 * tid - (tid & (size - 1));

    // Initialize the first half of shared memory with zeros to avoid "if (pos >= offset)"
    // condition evaluation
    smem[location] = 0;
    location += size;
    smem[location] = value;

    for (uint offset = 1; offset < size; offset <<= 1)
        smem[location] += smem[location - offset];

    return smem[location];
}

__attribute__((always_inline)) 
float scanWarpExclusive(uint tid, float value, uint size, __local float* smem)
{
    return scanWarpInclusive(tid, value, size, smem) - value;
}

__attribute__((always_inline)) 
float scanTopInclusive(uint tid, float value, __local float* smem)
{
    // Warp-level inclusive warp scan. Preserve scan result in each thread's
    // register space (variable 'warpResult')
    float warpResult = scanWarpInclusive(tid, value, WARP_SIZE, smem);

    // Sync to wait for warp scans to complete because smem is going to
    // be overwritten
    barrier(CLK_LOCAL_MEM_FENCE);

    // Save top elements of each warp for exclusive warp scan
    if ((tid & (WARP_SIZE - 1)) == WARP_SIZE - 1)
        smem[tid >> WARP_SIZE_LOG_2] = warpResult;

    // Wait for warp scans to complete
    barrier(CLK_LOCAL_MEM_FENCE);

    // Grab top warp elements
    float topValue = smem[tid];

    // Calculate exclsive scan and write back to shared memory
    smem[tid] = scanWarpExclusive(tid, topValue, NUM_THREADS >> WARP_SIZE_LOG_2, smem);

    // Wait for the result of top element scan
    barrier(CLK_LOCAL_MEM_FENCE);

    // Return updated warp scans with exclusive scan results
    return warpResult + smem[tid >> WARP_SIZE_LOG_2];
}

__attribute__((always_inline))
float scanTopExclusive(uint tid, float value, __local float* smem)
{
    return scanTopInclusive(tid, value, smem) - value;
}

__attribute__((always_inline))
void scanInclusive(uint tid, __local float* smem)
{
    // Each thread deals the number of "TEXELS_PER_THREAD" texels
    uint location = tid * TEXELS_PER_THREAD;
    // The lowest level (level-0) are stored in register space
    __private float privData[TEXELS_PER_THREAD];

    // Read back data from shared memory to register space
    #pragma unroll
    for (uint i = 0; i < TEXELS_PER_THREAD; ++i)
        privData[i] = location + i < IMAGE_SIZE ? smem[location + i] : 0;
    
    // Perform level-0 sum
    #pragma unroll
    for (uint i = 1; i < TEXELS_PER_THREAD; ++i)
        privData[i] += privData[i - 1];

    // Wait until all intra-thread operations finished
    barrier(CLK_LOCAL_MEM_FENCE);

    // Level-1 exclusive scan
    float topValue = privData[TEXELS_PER_THREAD - 1];
    float topResult = scanTopExclusive(tid, topValue, smem);

    // Wait until top level scan finished
    barrier(CLK_LOCAL_MEM_FENCE);

    // Propagate level-1 scan result to level-0, and then write to shared memory
    #pragma unroll
    for (uint i = 0; i < TEXELS_PER_THREAD; ++i)
    {
        if (location + i < IMAGE_SIZE)
            smem[location + i] = privData[i] + topResult;
    }

    // Wait until all write operations finished
    barrier(CLK_LOCAL_MEM_FENCE);
}

__attribute__((always_inline))
void scanInclusiveFiltering(uint tid, 
                            __local float* smem,
                            // Filtering parameters
                            float halfBoxFilterWidth,
                            float fracHalfBoxFilterWidth,
                            float invFracHalfBoxFilterWidth,
                            float rcpBoxFilterWidth )
{
    // Each thread deals the number of "TEXELS_PER_THREAD" texels
    uint location = tid * TEXELS_PER_THREAD;
    // The lowest level (level-0) are stored in register space
    __private float privData[TEXELS_PER_THREAD];

    // Calculating average values in the box window while performing level-0
    int L_pos = (int) ceil(location - 0.5f - halfBoxFilterWidth) - 1;
    int R_pos = (int)floor(location - 0.5f + halfBoxFilterWidth);

    float L_sum = smem[clamp(L_pos, 0, IMAGE_SIZE - 1)] * fracHalfBoxFilterWidth;
    float R_sum = smem[clamp(R_pos, 0, IMAGE_SIZE - 1)] * invFracHalfBoxFilterWidth;

    #pragma unroll
    for (uint i = 0; i < TEXELS_PER_THREAD; i ++)
    {
        float L_next = smem[clamp(L_pos + 1 + (int)i, 0, IMAGE_SIZE - 1)];
        float R_next = smem[clamp(R_pos + 1 + (int)i, 0, IMAGE_SIZE - 1)];

        privData[i] = (R_sum + R_next * fracHalfBoxFilterWidth) - (L_sum + L_next * invFracHalfBoxFilterWidth);
        privData[i] *= rcpBoxFilterWidth;

        L_sum += L_next;
        R_sum += R_next;
    }

    // Wait until all intra-thread operations finished
    barrier(CLK_LOCAL_MEM_FENCE);

    // Level-1 exclusive scan
    float topValue = privData[TEXELS_PER_THREAD - 1];
    float topResult = scanTopExclusive(tid, topValue, smem);

    // Wait until top level scan finished
    barrier(CLK_LOCAL_MEM_FENCE);

    // Propagate level-1 scan result to level-0, and then write to shared memory
    #pragma unroll
    for (uint i = 0; i < TEXELS_PER_THREAD; i ++)
    {
        if (location + i < IMAGE_SIZE)
            smem[location + i] = privData[i] + topResult;
    }

    // Wait until all write operations finished
    barrier(CLK_LOCAL_MEM_FENCE);
}

__attribute__((reqd_work_group_size(NUM_THREADS,1,1)))
__kernel void approxGaussian_pass_image(__read_only image2d_t src, 
                                        __write_only image2d_t dst,
                                        __local float* smem,
                                        uint numApproxPasses,
                                        float halfBoxFilterWidth,
                                        float fracHalfBoxFilterWidth,
                                        float invFracHalfBoxFilterWidth,
                                        float rcpBoxFilterWidth)
{
    const int tid = get_local_id(0);
    const int y = get_global_id(1);

    // Fetch the entire image row/column to local memory
    #pragma unroll
    for (int x = tid; x < IMAGE_SIZE; x += NUM_THREADS)
        smem[x] = read_imagef(src, sampler, IMAGE_COORDS(x, y)).x;

    barrier(CLK_LOCAL_MEM_FENCE);

    // Perform initial horizontal/vertical prefix-scan
    scanInclusive(tid, smem);

    for (uint i = 0; i < numApproxPasses; i ++)
    {
        scanInclusiveFiltering(tid, smem, halfBoxFilterWidth, 
                               fracHalfBoxFilterWidth, invFracHalfBoxFilterWidth, 
                               rcpBoxFilterWidth);
    }

    // Write the result performing last box-filtering
    #pragma unroll
    for (int x = tid; x < IMAGE_SIZE; x += NUM_THREADS)
    {
        float value = boxFilter1D(x, smem, halfBoxFilterWidth, 
                                  fracHalfBoxFilterWidth, rcpBoxFilterWidth);
        write_imagef(dst, IMAGE_COORDS(x, y), (float4)(value, 0, 0, 0));
    }
}
