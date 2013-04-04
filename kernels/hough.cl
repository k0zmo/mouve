#define NBINS 4

#if defined(USE_ATOMIC_COUNTERS)
#  pragma OPENCL EXTENSION cl_ext_atomic_counters_32 : enable
#  define counter_type counter32_t
#else
#  define counter_type volatile __global int*
#endif

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP;

__kernel void buildPointsList_basic(__read_only image2d_t src,
                                    __global uint* pointsList,
                                    counter_type pointsCount)
{
    int2 gid = (int2) { get_global_id(0), get_global_id(1) };
    int2 size = (int2) { get_image_width(src), get_image_height(src) };
    
    if(any(gid >= size))
        return;
        
    // read pixel
    float pix = read_imagef(src, sampler, gid).x;
        
    if(pix > 0.0f)
    {
        // pack non-zero coordinates as two 16-bit in one 32-bit
        uint coord = (gid.x << 16) | (gid.y & 0xFFFF);
        int idx = atomic_inc(pointsCount);
        pointsList[idx] = coord;
    }   
}

__kernel __attribute__((reqd_work_group_size(32,NBINS,1)))
void buildPointsList(__read_only image2d_t src,
                     __global uint* pointsList,
                     __global uint* pointsCount)
{
    int2 gid = (int2) { get_global_id(0), get_global_id(1) };
    int2 size = (int2) { get_image_width(src), get_image_height(src) };
    
    if(any(gid >= size))
        return;
        
    __local uint queue[NBINS][32];
    __local uint queueIndex[NBINS];
    __local uint globalQueueIndex[NBINS];
    
    int2 tid = { get_local_id(0), get_local_id(1) };
    // let first thread in a work group in each row zero queueIndex
    if(tid.x == 0)
        queueIndex[tid.y] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // read pixel
    float pix = read_imagef(src, sampler, gid).x;
        
    if(pix > 0.0f)
    {
        // pack non-zero coordinates as two 16-bit in one 32-bit
        uint coord = (gid.x << 16) | gid.y;
        // get location where to put pixel coord in local memory
        int idx = atomic_inc(&queueIndex[tid.y]);
        // push back pixel coord
        queue[tid.y][idx] = coord;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Let one thread calculate where to put points coords in global buffer
    if(tid.x == 0 && tid.y == 0)
    {
        uint totalSize = 0;
        #pragma unroll 
        for(int i = 0; i < NBINS; ++i)
        {
            globalQueueIndex[i] = totalSize;
            totalSize += queueIndex[i];
        }   
        
        uint globalOffset = atomic_add(pointsCount, totalSize);
        #pragma unroll
        for(int i = 0; i < NBINS; ++i)
            globalQueueIndex[i] += globalOffset;
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // copy local queues to a global one
    const uint qsize = queueIndex[tid.y];
    uint gidx = globalQueueIndex[tid.y] + tid.x;
    if(tid.x < qsize)
        pointsList[gidx] = queue[tid.y][tid.x];
}


/*
    Crashes AMD
    
//#define PIXELS_PER_THREAD 16

__kernel __attribute__((reqd_work_group_size(32,NBINS,1)))
void buildPointsList_x16(__read_only image2d_t src,
                         __global uint* pointsList,
                         __global uint* pointsCount)
{
    int2 gid = (int2) { get_global_id(0), get_global_id(1) };
    int2 size = (int2) { get_image_width(src), get_image_height(src) }; 
    int2 tid = { get_local_id(0), get_local_id(1) };

    __local uint queue[NBINS][32];
    __local uint queueIndex[NBINS];
    __local uint globalQueueIndex[NBINS];
    
    // __local uint globalQueueIndex[NBINS];
    // __local uint queueIndex[NBINS];
    // __local uint queue[NBINS][32];
        
    // let first thread in a work group in each row zero queueIndex
    if(tid.x == 0)
        queueIndex[tid.y] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if(gid.y < size.y)
    {
        int workGroupPitch = get_local_size(0) * PIXELS_PER_THREAD;
        int x = get_group_id(0) * workGroupPitch + tid.x;
        const int range = min(x + workGroupPitch, size.x);
        for( ; x < range; x += get_local_size(0))
        {
            // read pixel
            int2 c = (int2)(x, gid.y);
            float pix = read_imagef(src, sampler, c).x;
                
            if(pix > 0.0f)
            {
                // pack non-zero coordinates as two 16-bit in one 32-bit
                uint coord = (c.x << 16) | c.y;
                // get location where to put pixel coord in local memory
                int idx = atomic_inc(&queueIndex[tid.y]);
                // push back pixel coord
                queue[tid.y][idx] = coord;
            }
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Let one thread calculate where to put points coords in global buffer
    if(tid.x == 0 && tid.y == 0)
    {
        uint totalSize = 0;
        #pragma unroll
        for(int i = 0; i < NBINS; ++i)
        {
            globalQueueIndex[i] = totalSize;
            totalSize += queueIndex[i];
        }
            
        uint globalOffset = atomic_add(pointsCount, totalSize);
        #pragma unroll
        for(int i = 0; i < NBINS; ++i)
            globalQueueIndex[i] += globalOffset;
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    // copy local queues to a global one
    const uint qsize = queueIndex[tid.y];
    uint gidx = globalQueueIndex[tid.y] + tid.x;
    for(uint i = tid.x; i < qsize; i += get_local_size(0), gidx += get_local_size(0))
        pointsList[gidx] = queue[tid.y][i];
}
*/

#define THETA M_PI/180.0
        
__kernel void accumLines(__global uint* pointsLists,
                         __global uint* pointsCount,
                         __global int* accum,
                         const int numRho)
{
    int x = get_group_id(0);
    float angle = x * THETA;

    float cosVal;
    float sinVal = sincos(angle, &cosVal);
    
    int shift = (numRho - 1) / 2;
    uint nCount = pointsCount[0];
        
    for(uint i = get_local_id(0); i < nCount; i += get_local_size(0))
    {
        uint coords = pointsLists[i];
        uint x0 = coords >> 16;
        uint y0 = coords & 0xFFFF;
        
        int rho = convert_int_rte(x0*cosVal  + y0*sinVal) + shift;
        atomic_inc(&accum[mad24(x, numRho, rho)]);
    }
}

__kernel void accumLines_shared(__global uint* pointsLists,
                                __global uint* pointsCount,
                                __global int* accum,  /* opencv nie ma CV_32UC1 */
                                __local int* accumShared,
                                const int numRho)
{
    int x = get_group_id(0);
    int tid = get_local_id(0);
    int lsize = get_local_size(0);
    float angle = x * THETA;

    float cosVal;
    float sinVal = sincos(angle, &cosVal);
    
    int shift = (numRho - 1) / 2;
    int nCount = (int) pointsCount[0];

    // init sub-hough space with 0s
    for(uint i = tid; i < numRho; i += lsize)
        accumShared[i] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // each work group fill accumulator for a specific angle
    for(int i = tid; i < nCount; i += lsize)
    {
        uint coords = pointsLists[i];
        uint x0 = coords >> 16;
        uint y0 = coords & 0xFFFF;
        
        int rho = convert_int_rte(x0*cosVal  + y0*sinVal) + shift;  
        atomic_inc(&accumShared[rho]);
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Merge local sub-hough space
    for(int i = tid; i < numRho; i += lsize)
        accum[mad24(x, numRho, i)] = accumShared[i];
}

// 
//  Emulates read_image with clamp (to_border := 0)
//
__attribute__((always_inline))
int read_buffer_clamp(__global int* src,
                      int width, int height, int pitch,
                      int x, int y)
{
    if(y < 0 || y >= height - 1)
        return 0;
    if(x < 0 || x >= width - 1)
        return 0;
    return src[mad24(y, pitch, x)];
}
/*
__kernel void getLines(__global int* accum,
                       __global float2* lines,
                       counter_type pointsCount,
                       const int threshold,
                       const int maxLines,
                       const int numRho)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    if(gid.x >= numRho || gid.y >= 180)
        return;
        
#define FETCH(X, Y) read_buffer_clamp(accum, numRho, 180, numRho, X, Y)

    const float shift = (numRho - 1) * 0.5f;

    int v = FETCH(gid.x, gid.y);
    if(v > threshold 
        && v > FETCH(gid.x - 1, gid.y)
        && v > FETCH(gid.x + 1, gid.y)
        && v > FETCH(gid.x, gid.y - 1)
        && v > FETCH(gid.x, gid.y + 1))
    {
        int idx = atomic_inc(pointsCount);
        if(idx < maxLines)
        {
            float rho = gid.x - shift;
            float theta = gid.y * THETA;
            lines[idx] = (float2)(rho, theta);
        }
    }
}
*/
/*
__kernel void getLines(__global int* accum,
                       __global float2* lines,
                       counter_type pointsCount,
                       const int threshold,
                       const int maxLines,
                       const int numRho)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    if(gid.x >= numRho || gid.y >= 180)
        return;
        
#define FETCH(X, Y) read_buffer_clamp(accum, numRho, 180, numRho, X, Y)

    const float shift = (numRho - 1) * 0.5f;

    int v = FETCH(gid.x, gid.y);
    if(v > threshold 
        && v > FETCH(gid.x - 2, gid.y)
        && v > FETCH(gid.x - 1, gid.y)
        && v > FETCH(gid.x + 1, gid.y)
        && v > FETCH(gid.x + 2, gid.y)
        && v > FETCH(gid.x - 1, gid.y - 1)
        && v > FETCH(gid.x, gid.y - 1)
        && v > FETCH(gid.x + 1, gid.y - 1)
        && v > FETCH(gid.x, gid.y - 2)
        && v > FETCH(gid.x - 1, gid.y + 1)
        && v > FETCH(gid.x, gid.y + 1)
        && v > FETCH(gid.x + 1, gid.y + 1)
        && v > FETCH(gid.x, gid.y + 2))
    {
        int idx = atomic_inc(pointsCount);
        if(idx < maxLines)
        {
            float rho = gid.x - shift;
            float theta = gid.y * THETA;
            lines[idx] = (float2)(rho, theta);
        }
    }
}
*/

__kernel void getLines(__global int* accum,
                       __global float2* lines,
                       counter_type pointsCount,
                       const int threshold,
					   const int maxLines,
                       const int numRho)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    if(gid.x >= numRho || gid.y >= 180)
        return;
        
#define FETCH(X, Y) read_buffer_clamp(accum, numRho, 180, numRho, X, Y)

    const float shift = (numRho - 1) * 0.5f;

    int v = FETCH(gid.x, gid.y);
    if(v > threshold 
        && v >= FETCH(gid.x - 1, gid.y - 1)
        && v > FETCH(gid.x, gid.y - 1)
        && v >= FETCH(gid.x + 1, gid.y - 1)
        && v > FETCH(gid.x - 1, gid.y)
        && v > FETCH(gid.x + 1, gid.y)
        && v >= FETCH(gid.x - 1, gid.y + 1)
        && v > FETCH(gid.x, gid.y + 1)
        && v >= FETCH(gid.x + 1, gid.y + 1))
    {
        int idx = atomic_inc(pointsCount);
        if(idx < maxLines)
        {
            float rho = gid.x - shift;
            float theta = gid.y * THETA;
            lines[idx] = (float2)(rho, theta);
        }
    }
}