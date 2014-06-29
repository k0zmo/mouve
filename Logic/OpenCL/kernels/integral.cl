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

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

// +WARP_SIZE/2 to get rid of out-of-bound checks and +1 to avoid bank conflicts
#define SCAN_STRIDE (WARP_SIZE + WARP_SIZE/2 + 1)
#define NUM_THREADS 256
#define NUM_WARPS (NUM_THREADS / WARP_SIZE)

__attribute__((always_inline))
uint warp_inclusive_scan(const uint x,
                         // Assumes s is at least (WARP_SIZE+WARP_SIZE/2) 
                         // and is offseted by WARP_SIZE/2
                         __local volatile uint* s)
{
    // Initialize local memory so first WARP_SIZE/2 is 0s and the rest is x[]
    s[-WARP_SIZE/2] = 0;
    s[0] = x;

    uint sum = x;

    // Inclusive scan within a thread's warp (done in locksteps)
    #pragma unroll
    for(int offset = 1; offset < WARP_SIZE; offset <<= 1)
    {
        sum += s[-offset];
        s[0] = sum;
    }
    
    // synchronize to make all the totals available to the reduction code
    barrier(CLK_LOCAL_MEM_FENCE);

    return sum;
} 

__attribute__((always_inline))
uint workgroup_inclusive_scan(const uint tid,
                              const uint x,
                              __local volatile uint* restrict smem_scan,
                              __local volatile uint* restrict smem_totals)
{
    const int warp = tid / WARP_SIZE;
    const int lane = (WARP_SIZE - 1) & tid;

    // Perform warp scan - each warp from work group does scan within its extent
    __local volatile uint* s = smem_scan + SCAN_STRIDE*warp + lane + WARP_SIZE/2;
    const uint warpSum = warp_inclusive_scan(x, s);
    
    // Merge warp results using exclusive scan of the last elements from each warp scan.
    // This is done by first NUM_WARPS threads
    if(tid < NUM_WARPS)
    {
        // Grab the last element from warp scan sequence.
        uint x = smem_scan[SCAN_STRIDE*tid + WARP_SIZE/2 + WARP_SIZE - 1];

        __local volatile uint* s2 = smem_totals + NUM_WARPS/2 + tid;
        s2[-NUM_WARPS/2] = 0;
        s2[0] = x;

        uint sum = x;

        // Inclusive scan (assumption: done in locksteps if NUM_WARPS < WARP_SIZE)
        #pragma unroll
        for(int offset = 1; offset < NUM_WARPS; offset <<= 1)
        {
            sum += s2[-offset];
            s2[0] = sum;
        }

        // Make it exclusive scan
        // Ex: Assume 256-vector of 1's and warp_size=64.
        // Then sum = [64,128,192,256], x=[64,64,64,64]
        // and sum - x = [0,64,128,192]
        smem_totals[tid] = sum - x;
    }

    // Synchronize to make the smem_totals block available to all warps.
    barrier(CLK_LOCAL_MEM_FENCE);

    return warpSum + smem_totals[warp];
}

__attribute__((reqd_work_group_size(NUM_THREADS,1,1)))
__kernel void multiscan_horiz_image(__read_only image2d_t src, 
                                    __write_only image2d_t dst)
{
    __local volatile uint smem_scan[NUM_WARPS * SCAN_STRIDE];
    __local volatile uint smem_totals[NUM_WARPS + NUM_WARPS/2];

    const int y = get_global_id(1);
    const int tid = get_local_id(0);

    const int cols = get_image_width(src);
    const int iters = (cols + NUM_THREADS - 1)/ NUM_THREADS;
    int prevMax = 0;

    for(int i = 0; i < iters; ++i)
    {
        int lx = tid + i*NUM_THREADS;

        uint x = convert_uint(read_imagef(src, sampler, (int2)(lx, y)).x * 255.0f);
        uint sum = workgroup_inclusive_scan(tid, x, smem_scan, smem_totals);

        if(lx < cols)
        {
            uint v = sum + prevMax;
            write_imageui(dst, (int2)(lx,y), (uint4)(v,0,0,0));
        }
        prevMax += smem_totals[NUM_WARPS + NUM_WARPS/2 - 1];
    }
}

__attribute__((reqd_work_group_size(NUM_THREADS,1,1)))
__kernel void multiscan_vert_image(__read_only image2d_t src, 
                                   __write_only image2d_t dst)
{
    __local volatile uint smem_scan[NUM_WARPS * SCAN_STRIDE];
    __local volatile uint smem_totals[NUM_WARPS + NUM_WARPS/2];

    const int x = get_global_id(1);
    const int tid = get_local_id(0);

    const int rows = get_image_height(src);
    const int iters = (rows + NUM_THREADS - 1)/ NUM_THREADS;
    int prevMax = 0;    

    for(int i = 0; i < iters; ++i)
    {
        int ly = tid + i*NUM_THREADS;

        uint y = read_imageui(src, sampler, (int2)(x, ly)).x;
        uint sum = workgroup_inclusive_scan(tid, y, smem_scan, smem_totals);

        if(ly < rows)
        {
            // dst is one pixel bigger with 0s on first row and first column
            // that means we need to shift writing by one
            uint v = sum + prevMax;
            write_imageui(dst, (int2)(x+1, ly+1), (uint4)(v,0,0,0));
        }
        prevMax += smem_totals[NUM_WARPS + NUM_WARPS/2 - 1];
    }
}

__attribute__((reqd_work_group_size(NUM_THREADS,1,1)))
__kernel void multiscan_horiz_image2(__read_only image2d_t src, 
                                     __write_only image2d_t dst)
{
    __local volatile uint smem_scan[NUM_WARPS * SCAN_STRIDE];
    __local volatile uint smem_totals[NUM_WARPS + NUM_WARPS/2];

    const int y = get_global_id(1);
    const int tid = get_local_id(0);
    const int warp = tid / WARP_SIZE;
    const int lane = (WARP_SIZE - 1) & tid;

    const int cols = get_image_width(src);
    const int iters = (cols + NUM_THREADS - 1)/ NUM_THREADS;
    int prevMax = 0;

    __local volatile uint* s = smem_scan + SCAN_STRIDE*warp + lane + WARP_SIZE/2;

    for(int i = 0; i < iters; ++i)
    {
        int lx = tid + i*NUM_THREADS;

        // read from global memory (check bounds first)
        uint x = convert_uint(read_imagef(src, sampler, (int2)(lx, y)).x * 255.0f);
        s[-WARP_SIZE/2] = 0;
        s[0] = x;

        // run inclusive scan on each warp's data
        uint sum = x;

        #pragma unroll
        for(int offset = 1; offset < WARP_SIZE; offset <<= 1)
        {
            sum += s[-offset];
            s[0] = sum;
        }
        
        // synchronize to make all the totals available to the reduction code
        barrier(CLK_LOCAL_MEM_FENCE);
        
        if(tid < NUM_WARPS)
        {
            // grab the block total for the tid'th block. This is the last element
            // in the block's scanned sequence. This operation avoids bank conflicts
            uint total = smem_scan[SCAN_STRIDE*tid + WARP_SIZE/2 + WARP_SIZE - 1];

            smem_totals[tid] = 0;
            __local volatile uint* s2 = smem_totals + NUM_WARPS/2 + tid;
            uint totalsSum = total;
            s2[0] = total;

            // exclusive scan
            #pragma unroll
            for(int offset = 1; offset < NUM_WARPS; offset <<= 1)
            {
                totalsSum += s2[-offset];
                s2[0] = totalsSum;
            }
            // Assume 256-vector of 1's and warp_size=64.
            // Then totalsSum = [64,128,192,256], total=[64,64,64,64]
            // and totalsSum-total = [0,64,128,192]
            smem_totals[tid] = totalsSum - total;
        }

        // synchronize to make the block smem_scan available to all warps.
        barrier(CLK_LOCAL_MEM_FENCE);

        if(lx < cols)
        {
            uint v = sum + smem_totals[warp] + prevMax;
            write_imageui(dst, (int2)(lx,y), (uint4)(v,0,0,0));
        }
        prevMax += smem_totals[NUM_WARPS + NUM_WARPS/2 - 1];
    }
}

__attribute__((reqd_work_group_size(NUM_THREADS,1,1)))
__kernel void multiscan_vert_image2(__read_only image2d_t src, 
                                    __write_only image2d_t dst)
{
    __local volatile uint smem_scan[NUM_WARPS * SCAN_STRIDE];
    __local volatile uint smem_totals[NUM_WARPS + NUM_WARPS/2];

    const int x = get_global_id(1);
    const int tid = get_local_id(0);
    const int warp = tid / WARP_SIZE;
    const int lane = (WARP_SIZE - 1) & tid;

    const int rows = get_image_height(src);
    const int iters = (rows + NUM_THREADS - 1)/ NUM_THREADS;
    int prevMax = 0;    

    __local volatile uint* s = smem_scan + SCAN_STRIDE*warp + lane + WARP_SIZE/2;

    for(int i = 0; i < iters; ++i)
    {
        int ly = tid + i*NUM_THREADS;

        // read from global memory (check bounds first)
        uint y = read_imageui(src, sampler, (int2)(x, ly)).x;
        s[-WARP_SIZE/2] = 0;
        s[0] = y;

        // run inclusive scan on each warp's data
        uint sum = y;

        #pragma unroll
        for(int offset = 1; offset < WARP_SIZE; offset <<= 1)
        {
            sum += s[-offset];
            s[0] = sum;
        }
        
        // synchronize to make all the totals available to the reduction code
        barrier(CLK_LOCAL_MEM_FENCE);
        
        if(tid < NUM_WARPS)
        {
            // grab the block total for the tid'th block. This is the last element
            // in the block's scanned sequence. This operation avoids bank conflicts
            uint total = smem_scan[SCAN_STRIDE*tid + WARP_SIZE/2 + WARP_SIZE - 1];

            smem_totals[tid] = 0;
            __local volatile uint* s2 = smem_totals + NUM_WARPS/2 + tid;
            uint totalsSum = total;
            s2[0] = total;

            // exclusive scan
            #pragma unroll
            for(int offset = 1; offset < NUM_WARPS; offset <<= 1)
            {
                totalsSum += s2[-offset];
                s2[0] = totalsSum;
            }
            // Assume 256-vector of 1's and warp_size=64.
            // Then totalsSum = [64,128,192,256], total=[64,64,64,64]
            // and totalsSum-total = [0,64,128,192]
            smem_totals[tid] = totalsSum - total;
        }

        // synchronize to make the block smem_scan available to all warps.
        barrier(CLK_LOCAL_MEM_FENCE);

        if(ly < rows)
        {
            // dst is one pixel bigger with 0s on first row and first column
            // that means we need to shift writing by one
            uint v = sum + smem_totals[warp] + prevMax;
            write_imageui(dst, (int2)(x+1, ly+1), (uint4)(v,0,0,0));
        }
        prevMax += smem_totals[NUM_WARPS + NUM_WARPS/2 - 1];
    }
}
