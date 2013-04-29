__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

// +WARP_SIZE/2 to get rid of ifs and +1 to avoid bank conflicts
#define SCAN_STRIDE (WARP_SIZE + WARP_SIZE/2 + 1)
#define NUM_THREADS 256
#define NUM_WARPS (NUM_THREADS / WARP_SIZE)

__attribute__((reqd_work_group_size(NUM_THREADS,1,1)))
__kernel void multiscan_horiz_image(__read_only image2d_t src, 
                                    __write_only image2d_t dst)
{
    __local volatile uint smem_scan[NUM_WARPS * SCAN_STRIDE];
    __local volatile uint smem_totals[NUM_WARPS + NUM_WARPS/2];

    int y = get_global_id(1);
    int tid = get_local_id(0);
    int warp = tid / WARP_SIZE;
    int lane = (WARP_SIZE - 1) & tid;

    int cols = get_image_width(src);
    int prevMax = 0;
    int iters = cols / NUM_THREADS;
    if(cols % NUM_THREADS != 0)
        ++iters;

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
__kernel void multiscan_vert_image(__read_only image2d_t src, 
                                   __write_only image2d_t dst)
{
    __local volatile uint smem_scan[NUM_WARPS * SCAN_STRIDE];
    __local volatile uint smem_totals[NUM_WARPS + NUM_WARPS/2];

    int x = get_global_id(1);
    int tid = get_local_id(0);
    int warp = tid / WARP_SIZE;
    int lane = (WARP_SIZE - 1) & tid;

    int rows = get_image_height(src);
    int prevMax = 0;
    int iters = rows / NUM_THREADS;
    if(rows % NUM_THREADS != 0)
        ++iters;

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


#undef SCAN_STRIDE
#define SCAN_STRIDE (WARP_SIZE + WARP_SIZE/2)

__attribute__((reqd_work_group_size(WARP_SIZE,1,1)))
__kernel void scan_horiz_image(__read_only image2d_t src,
                               __write_only image2d_t dst)
{
    // Each work group is responsible for scanning a row
    __local uint smem[SCAN_STRIDE];
    __local uint smem2[SCAN_STRIDE];

    int rowId = get_global_id(1);
    int tid = get_local_id(0);
    int cols = get_image_width(src);

    // Initialize out-of-bounds elements with zeros
    if(tid < WARP_SIZE/2)
    {
        smem[tid] = 0; 
        smem2[tid] = 0;
    }

    // Start past out-of-bounds elements
    tid += WARP_SIZE/2;

    // This value needs to be added to the local data.  It's the highest
    // value from the prefix scan of the previous elements in the row
    uint prevMaxVal = 0;

    int iterations = cols/WARP_SIZE;
    if(cols % WARP_SIZE != 0)
        ++iterations;

    for(int i = 0; i < iterations; ++i)
    {
        int columnOffset = i*WARP_SIZE +get_local_id(0);

        smem[tid] = convert_uint(read_imagef(src, sampler, (int2)(columnOffset, rowId)).x * 255.0f);
        barrier(CLK_LOCAL_MEM_FENCE);

        // 1
        smem2[tid] = smem[tid] + smem[tid-1];
        barrier(CLK_LOCAL_MEM_FENCE);
        // 2
        smem[tid] = smem2[tid] + smem2[tid-2];
        barrier(CLK_LOCAL_MEM_FENCE);
        // 4
        smem2[tid] = smem[tid] + smem[tid-4];
        barrier(CLK_LOCAL_MEM_FENCE);
        // 8
        smem[tid] = smem2[tid] + smem2[tid-8];
        barrier(CLK_LOCAL_MEM_FENCE);
        // 16
        smem2[tid] = smem[tid] + smem[tid-16];
        barrier(CLK_LOCAL_MEM_FENCE);
        // 32
        smem[tid] = smem2[tid] + smem2[tid-32];
        barrier(CLK_LOCAL_MEM_FENCE);

        if(columnOffset < cols) 
            write_imageui(dst, (int2)(columnOffset, rowId), (uint4)(smem[tid] + prevMaxVal, 0, 0, 0));
        prevMaxVal += smem[WARP_SIZE + WARP_SIZE/2 - 1];
    }
}

__attribute__((reqd_work_group_size(WARP_SIZE,1,1)))
__kernel void scan_vert_image(__read_only image2d_t src,
                              __write_only image2d_t dst)
{
    // Each work group is responsible for scanning a row
    __local uint smem[SCAN_STRIDE];
    __local uint smem2[SCAN_STRIDE];

    int colId = get_global_id(1);
    int tid = get_local_id(0);
    int rows = get_image_height(src);

    // Initialize out-of-bounds elements with zeros
    if(tid < WARP_SIZE/2)
    {
        smem[tid] = 0; 
        smem2[tid] = 0;
    }

    // Start past out-of-bounds elements
    tid += WARP_SIZE/2;

    // This value needs to be added to the local data.  It's the highest
    // value from the prefix scan of the previous elements in the row
    uint prevMaxVal = 0;

    int iterations = rows/WARP_SIZE;
    if(rows % WARP_SIZE != 0)
        ++iterations;

    for(int i = 0; i < iterations; ++i)
    {
        int rowOffset = i*WARP_SIZE + get_local_id(0);

        smem[tid] = read_imageui(src, sampler, (int2)(colId, rowOffset)).x;
        barrier(CLK_LOCAL_MEM_FENCE);

        // 1
        smem2[tid] = smem[tid] + smem[tid-1];
        barrier(CLK_LOCAL_MEM_FENCE);
        // 2
        smem[tid] = smem2[tid] + smem2[tid-2];
        barrier(CLK_LOCAL_MEM_FENCE);
        // 4
        smem2[tid] = smem[tid] + smem[tid-4];
        barrier(CLK_LOCAL_MEM_FENCE);
        // 8
        smem[tid] = smem2[tid] + smem2[tid-8];
        barrier(CLK_LOCAL_MEM_FENCE);
        // 16
        smem2[tid] = smem[tid] + smem[tid-16];
        barrier(CLK_LOCAL_MEM_FENCE);
        // 32
        smem[tid] = smem2[tid] + smem2[tid-32];
        barrier(CLK_LOCAL_MEM_FENCE);

        if(rowOffset < rows) 
            // dst is one pixel bigger with 0s on first row and first column
            // that means we need to shift writing by one
            write_imageui(dst, (int2)(colId+1, rowOffset+1), (uint4)(smem[tid] + prevMaxVal, 0, 0, 0));
        prevMaxVal += smem[WARP_SIZE + WARP_SIZE/2 - 1];
    }
}
