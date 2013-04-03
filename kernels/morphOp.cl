//#define ERODE_OP

#if defined(ERODE_OP)

#define PIX_INF 1.0f
#define PIX_INF_UCHAR 255

__attribute__((always_inline))
float morphOpf(float a, float b)
{
    return min(a, b);
}

__attribute__((always_inline))
uchar morphOp(uchar a, uchar b)
{
    return min(a, b);
}


#elif defined(DILATE_OP)

#define PIX_INF 0.0f
#define PIX_INF_UCHAR 0

__attribute__((always_inline))
float morphOpf(float a, float b)
{
    return max(a, b);
}

__attribute__((always_inline))
uchar morphOp(uchar a, uchar b)
{
    return max(a, b);
}

#endif

__constant sampler_t sampler_edge = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;

// Morph operator, image format: CL_UNORM_INT8, CL_R
__kernel void morphOp_image_unorm(__read_only image2d_t src,
                                  __write_only image2d_t dst,
                                  __constant int2* coords,
                                  const int coordsSize)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    int2 size = { get_image_width(src), get_image_height(src) };
    // Skip redundant threads that don't correspond to valid pixels
    if(any(gid >= size))
        return;
        
    float pix = PIX_INF;
    for(int i = 0; i < coordsSize; ++i)
    {
        int2 coord = coords[i] + gid;
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord).x);
    }
    write_imagef(dst, gid, (float4)(pix));
}

// Morph operator, image format: CL_UNORM_INT8, CL_R
__kernel void morphOp_image_unorm_unroll2(__read_only image2d_t src,
                                          __write_only image2d_t dst,
                                          __constant int4* coords,
                                          const int coordsSize)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    int2 size = { get_image_width(src), get_image_height(src) };
    // Skip redundant threads that don't correspond to valid pixels
    if(any(gid >= size))
        return;
        
    float pix = PIX_INF;
    int c2 = coordsSize >> 1;
    for(int i = 0; i < c2; ++i)
    {
        int4 coord = coords[i] + (int4)(gid, gid);
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord.xy).x);
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord.zw).x);
    }
    if(coordsSize % 2)
    {
        int2 coord = ((__constant int2*)coords)[coordsSize-1] + gid;
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord).x);
    }
    write_imagef(dst, gid, (float4)(pix));
}

// Morph operator, image format: CL_UNORM_INT8, CL_R
__kernel void morphOp_image_unorm_unroll4(__read_only image2d_t src,
                                          __write_only image2d_t dst,
                                          __constant int4* coords,
                                          const int coordsSize)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    int2 size = { get_image_width(src), get_image_height(src) };
    // Skip redundant threads that don't correspond to valid pixels
    if(any(gid >= size))
        return;
        
    float pix = PIX_INF;
    int c2 = (coordsSize >> 1) - 1;
    int i = 0;
    for( ; i < c2; i += 2)
    {
        int4 coord0 = coords[i] + (int4)(gid, gid);
        int4 coord1 = coords[i+1] + (int4)(gid, gid);
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord0.xy).x);
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord0.zw).x);
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord1.xy).x);
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord1.zw).x);
    }
    i *= 2;
    for( ; i < coordsSize; ++i)
    {
        int2 coord = ((__constant int2*)coords)[i] + gid;
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord).x);
    }
    write_imagef(dst, gid, (float4)(pix));
}

#if defined(COORDS_SIZE)

// Morph operator, image format: CL_UNORM_INT8, CL_R
__kernel void morphOp_image_unorm_unroll(__read_only image2d_t src,
                                         __write_only image2d_t dst,
                                         __constant int4* coords,
                                         const int coordsSize /* dummy */)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    int2 size = { get_image_width(src), get_image_height(src) };
    // Skip redundant threads that don't correspond to valid pixels
    if(any(gid >= size))
        return;
        
    float pix = PIX_INF;
    int c2 = COORDS_SIZE >> 1;
    #pragma unroll
    for(int i = 0; i < c2; ++i)
    {
        int4 coord = coords[i] + (int4)(gid, gid);
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord.xy).x);
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord.zw).x);
    }
    if(COORDS_SIZE % 2)
    {
        int2 coord = ((__constant int2*)coords)[COORDS_SIZE-1] + gid;
        pix = morphOpf(pix, read_imagef(src, sampler_edge, coord).x);
    }
    write_imagef(dst, gid, (float4)(pix));
}

#endif

__attribute__((always_inline))
void contextToLocalMemory_image(__read_only image2d_t src,
                                int kradx, int krady,
                                __local uchar* sharedBlock,
                                int sharedWidth, int sharedHeight)
{
    int2 lid = { get_local_id(0), get_local_id(1) };
    int2 localSize = { get_local_size(0), get_local_size(1) };
    int2 groupId = { get_group_id(0), get_group_id(1) };
    // First byte Id in current work group
    // Contant across all work items in a work group
    int2 groupStartId = mul24(groupId, localSize);

    for(int y = lid.y; y < sharedHeight; y += localSize.y)
    {
        int ycoord = groupStartId.y + y - krady;
        for(int x = lid.x; x < sharedWidth; x += localSize.x)
        {
            int xcoord = groupStartId.x + x - kradx;
            int2 ic = { xcoord , ycoord };
            float pixf = read_imagef(src, sampler_edge, ic).x;
            uchar pix = convert_uchar(pixf * 255.0f);
            sharedBlock[mad24(y, sharedWidth, x)] = pix;
        }
    }
}

/*
    Works only when kradx <= work_grop_size_x and krady <= work_group_size_y
    It's a bit faster for small contexts.
*/
__attribute__((always_inline))
void contextToLocalMemory_image2(__read_only image2d_t src,
                                int kradx, int krady,
                                __local uchar* sharedBlock,
                                int sharedWidth, int sharedHeight)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    int2 lid = { get_local_id(0), get_local_id(1) };
    int2 lsize = { get_local_size(0), get_local_size(1) };

#define SMEM(X, Y) sharedBlock[mad24(Y, sharedWidth, X)]

    SMEM(lid.x+kradx, lid.y+krady) = convert_uchar(read_imagef(src, sampler_edge, gid).x * 255.0f);

    // borders
    if(lid.x < kradx)
    {
        // left
        SMEM(lid.x, lid.y+krady) = convert_uchar(read_imagef(src, sampler_edge, gid + (int2)(-kradx, 0)).x * 255.0f);
        // right
        SMEM(lid.x+lsize.x+kradx, lid.y+krady) = convert_uchar(read_imagef(src, sampler_edge, gid + (int2)(lsize.x, 0)).x * 255.0f);
    }
    if(lid.y < krady)
    {
        // top
        SMEM(lid.x+kradx, lid.y) = convert_uchar(read_imagef(src, sampler_edge, gid + (int2)(0,-krady)).x * 255.0f);
        // bottom
        SMEM(lid.x+kradx, lid.y+lsize.y+krady) = convert_uchar(read_imagef(src, sampler_edge, gid + (int2)(0,lsize.y)).x * 255.0f);
    }
    // corners
    if((lid.x < kradx) && (lid.y < krady))
    {
        // topleft
        SMEM(lid.x, lid.y) = convert_uchar(read_imagef(src, sampler_edge, gid + (int2)(-kradx,-krady)).x * 255.0f);
        // bottomleft
        SMEM(lid.x, lid.y+lsize.y+krady) = convert_uchar(read_imagef(src, sampler_edge, gid + (int2)(-kradx,lsize.y)).x * 255.0f);
        // topright
        SMEM(lid.x+lsize.x+kradx, lid.y) = convert_uchar(read_imagef(src, sampler_edge, gid + (int2)(lsize.x,-krady)).x * 255.0f);
        // bottomright
        SMEM(lid.x+lsize.x+kradx, lid.y+lsize.y+krady) = convert_uchar(read_imagef(src, sampler_edge, gid + (int2)(lsize.x,lsize.y)).x * 255.0f);
    }
#undef SMEM
}

// Morph operator, image format: CL_UNORM_INT8, CL_R
__kernel void morphOp_image_unorm_local(__read_only image2d_t src,
                                        __write_only image2d_t dst,
                                        __constant int2* coords,
                                        const int coordsSize, int kradx, int krady,
                                        __local uchar* sharedBlock,
                                        int sharedWidth, int sharedHeight)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    int2 lid = { get_local_id(0), get_local_id(1) };
    int2 size = { get_image_width(src), get_image_height(src) };
    // Skip redundant threads that don't correspond to valid pixels
    if(any(gid >= size))
        return;

    // Move context to local memory
    contextToLocalMemory_image(src, kradx, krady, sharedBlock, sharedWidth, sharedHeight);
    barrier(CLK_LOCAL_MEM_FENCE);

    // Perform minimal filtering using cached context from local memory
    uchar pix = PIX_INF_UCHAR;
    for(int i = 0; i < coordsSize; ++i)
    {
        int2 ic = coords[i] + lid;
        pix = morphOp(pix, sharedBlock[mad24(ic.y+krady, sharedWidth, ic.x+kradx)]);
    }
    float pixf = convert_float(pix) / 255.0f;
    write_imagef(dst, gid, (float4)(pixf));
}

// Morph operator, image format: CL_UNORM_INT8, CL_R
__kernel void morphOp_image_unorm_local_unroll2(__read_only image2d_t src,
                                                __write_only image2d_t dst,
                                                __constant int4* coords,
                                                const int coordsSize, int kradx, int krady,
                                                __local uchar* sharedBlock,
                                                int sharedWidth, int sharedHeight)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    int2 lid = { get_local_id(0), get_local_id(1) };
    int2 size = { get_image_width(src), get_image_height(src) };
    if(any(gid >= size))
        return;

    contextToLocalMemory_image(src, kradx, krady, sharedBlock, sharedWidth, sharedHeight);
    barrier(CLK_LOCAL_MEM_FENCE);

    uchar pix = PIX_INF_UCHAR;
    int c2 = coordsSize >> 1;
    for(int i = 0; i < c2; ++i)
    {
        int4 ic = coords[i] + (int4)(lid, lid);
        pix = morphOp(pix, sharedBlock[mad24(ic.y+krady, sharedWidth, ic.x+krady)]);
        pix = morphOp(pix, sharedBlock[mad24(ic.w+krady, sharedWidth, ic.z+kradx)]);
    }
    if(coordsSize % 2)
    {
        int2 ic = ((__constant int2*)coords)[coordsSize-1] + lid;
        pix = morphOp(pix, sharedBlock[mad24(ic.y+krady, sharedWidth, ic.x+kradx)]);
    }
    float pixf = convert_float(pix) / 255.0f;
    write_imagef(dst, gid, (float4)(pixf));
}

#if defined(COORDS_SIZE)

// Morph operator, image format: CL_UNORM_INT8, CL_R
__kernel void morphOp_image_unorm_local_unroll(__read_only image2d_t src,
                                               __write_only image2d_t dst,
                                               __constant int4* coords,
                                               const int coordsSize, int kradx, int krady,
                                               __local uchar* sharedBlock,
                                               int sharedWidth, int sharedHeight)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    int2 lid = { get_local_id(0), get_local_id(1) };
    int2 size = { get_image_width(src), get_image_height(src) };
    if(any(gid >= size))
        return;

    contextToLocalMemory_image(src, kradx, krady, sharedBlock, sharedWidth, sharedHeight);
    barrier(CLK_LOCAL_MEM_FENCE);

    uchar pix = PIX_INF_UCHAR;
    int c2 = COORDS_SIZE >> 1;
    #pragma unroll
    for(int i = 0; i < c2; ++i)
    {
        int4 ic = coords[i] + (int4)(lid, lid);
        pix = morphOp(pix, sharedBlock[mad24(ic.y+krady, sharedWidth, ic.x+kradx)]);
        pix = morphOp(pix, sharedBlock[mad24(ic.w+krady, sharedWidth, ic.z+kradx)]);
    }
    if(COORDS_SIZE % 2)
    {
        int2 ic = ((__constant int2*)coords)[COORDS_SIZE-1] + lid;
        pix = morphOp(pix, sharedBlock[mad24(ic.y+krady, sharedWidth, ic.x+kradx)]);
    }
    float pixf = convert_float(pix) / 255.0f;
    write_imagef(dst, gid, (float4)(pixf));
}

#endif