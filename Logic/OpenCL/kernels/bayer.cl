__attribute__((always_inline))
float read_imagef_bayer(image2d_t src, 
                        sampler_t sampler, 
                        int2 coords, 
                        int2 size)
{
    // Isn't really necessary but costs close to nothing
    int maxx = size.x - 1;
    int maxy = size.y - 1;
    coords.x = coords.x < 0 ? 1 : coords.x;
    coords.x = coords.x > maxx ? maxx - 1 : coords.x;
    coords.y = coords.y < 0 ? 1 : coords.y;
    coords.y = coords.y > maxy ? maxy - 1 : coords.y;           
    return read_imagef(src, sampler, coords).x;
}

__attribute__((always_inline))
float3 convert_bayer2rgb(__read_only image2d_t src,
                         sampler_t sampler,
                         bool x_even,
                         bool y_even)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    int2 size = get_image_dim(src);
    __private float v[9];
    
    // fetch pixel with its full 3x3 context
    #pragma unroll
    for(int y = 0; y < 3; ++y) 
    {
        #pragma unroll
        for(int x = 0; x < 3; ++x)
        {
            int2 coords = gid + (int2)(x-1, y-1);       
            v[mad24(y,3,x)] = read_imagef_bayer(src, sampler, coords, size);
        }
    }

    // Dla postaci gdzie G jest 4
    float r = v[4];
    float g = (v[3] + v[5] + v[1] + v[7]) * 0.25f;
    float b = (v[0] + v[2] + v[6] + v[8]) * 0.25f;
    float3 out1 = (float3)(r,g,b);

    // Dla postaci gdzie G jest 5 (lub jeden) a reszty po 2
    r = (v[3] + v[5]) * 0.5f;
    g = v[4];
    b = (v[1] + v[7]) * 0.5f;
    float3 out2 = (float3)(r,g,b);

    return x_even ?
        (y_even ? out1.xyz : out2.zyx) :
        (y_even ? out2.xyz : out1.zyx);
}

__attribute__((always_inline))
void contextToLocalMemory_image2(__read_only image2d_t src,
                                 sampler_t sampler,
                                 int kradx, int krady,
                                 __local float* sharedBlock,
                                 int sharedWidth, int sharedHeight)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    int2 lid = { get_local_id(0), get_local_id(1) };
    int2 lsize = { get_local_size(0), get_local_size(1) };
    int2 size = get_image_dim(src);

#define SMEM(X, Y) sharedBlock[mad24(Y, sharedWidth, X)]

    SMEM(lid.x+kradx, lid.y+krady) = read_imagef(src, sampler, gid).x;

    // borders
    if(lid.x < kradx)
    {
        // left
        SMEM(lid.x, lid.y+krady) = read_imagef_bayer(src, sampler, gid + (int2)(-kradx, 0), size);
        // right
        SMEM(lid.x+lsize.x+kradx, lid.y+krady) = read_imagef_bayer(src, sampler, gid + (int2)(lsize.x, 0), size);
    }
    if(lid.y < krady)
    {
        // top
        SMEM(lid.x+kradx, lid.y) = read_imagef_bayer(src, sampler, gid + (int2)(0,-krady), size);
        // bottom
        SMEM(lid.x+kradx, lid.y+lsize.y+krady) = read_imagef_bayer(src, sampler, gid + (int2)(0,lsize.y), size);
    }
    // corners
    if((lid.x < kradx) && (lid.y < krady))
    {
        // topleft
        SMEM(lid.x, lid.y) = read_imagef_bayer(src, sampler, gid + (int2)(-kradx,-krady), size);
        // bottomleft
        SMEM(lid.x, lid.y+lsize.y+krady) = read_imagef_bayer(src, sampler, gid + (int2)(-kradx,lsize.y), size);
        // topright
        SMEM(lid.x+lsize.x+kradx, lid.y) = read_imagef_bayer(src, sampler, gid + (int2)(lsize.x,-krady), size);
        // bottomright
        SMEM(lid.x+lsize.x+kradx, lid.y+lsize.y+krady) = read_imagef_bayer(src, sampler, gid + (int2)(lsize.x,lsize.y), size);
    }
#undef SMEM
}

float3 convert_bayer2rgb_local(__local float* smem,
                               int smemWidth, 
                               bool x_even, bool y_even)
{
    int2 lid = { get_local_id(0), get_local_id(1) };

#define SMEM(X, Y) smem[mad24(Y+1 + lid.y, smemWidth, X+1 + lid.x)]
    float v4   = SMEM( 0, 0);
    float sum0 = SMEM(-1, 0) + SMEM(1, 0);
    float sum1 = SMEM( 0,-1) + SMEM(0, 1);
    float sum2 = SMEM(-1,-1) + SMEM(1,-1);
    float sum3 = SMEM(-1, 1) + SMEM(1, 1);
#undef SMEM

    // Dla postaci gdzie G jest 4
    float r = v4;
    float g = (sum0 + sum1) * 0.25f;
    float b = (sum2 + sum3) * 0.25f;
    float3 out1 = (float3)(r,g,b);

    // Dla postaci gdzie G jest 5 (lub jeden) a reszty po 2
    r = (sum0) * 0.5f;
    g = v4;
    b = (sum1) * 0.5f;
    float3 out2 = (float3)(r,g,b);
    
    return x_even ?
        (y_even ? out1.xyz : out2.zyx) :
        (y_even ? out2.xyz : out1.zyx);   
}

__constant float3 greyscale = { 0.2989f, 0.5870f, 0.1140f };
__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP;

#if defined(TEMPLATES_SUPPORTED)

template<bool xodd, bool yodd, bool toGray>
__kernel void bayerKernel(__read_only image2d_t src,
                          __write_only image2d_t dst,
                          float3 gains)
{
    int2 gid = (int2) { get_global_id(0), get_global_id(1) };
    int2 size = get_image_dim(src);
    if(any(gid >= size))
        return;
    bool x_odd = gid.x & 0x01; bool y_odd = gid.y & 0x01;
    float3 rgb = convert_bayer2rgb(src, sampler, x_odd == xodd, y_odd == yodd);
    rgb *= gains.xyz;

    if(toGray)
    {
        float gray = dot(rgb, greyscale);
        write_imagef(dst, gid, (float4)(gray));
    }
    else
    {
        write_imagef(dst, gid, (float4)(rgb, 1.0f));
    }
}

template<bool xodd, bool yodd, bool toGray>
__kernel void bayerKernel_local(__read_only image2d_t src,
                                __write_only image2d_t dst,
                                float3 gains,
                                __local float* smem,
                                int smemWidth, int smemHeight)
{
    int2 gid = (int2) { get_global_id(0), get_global_id(1) };
    int2 lid = { get_local_id(0), get_local_id(1) };
    int2 size = get_image_dim(src);
    if(any(gid >= size))
        return;
    bool x_odd = gid.x & 0x01; bool y_odd = gid.y & 0x01;
    
    contextToLocalMemory_image2(src, sampler, 1, 1, smem, smemWidth, smemHeight);
    barrier(CLK_LOCAL_MEM_FENCE); 
    
    float3 rgb = convert_bayer2rgb_local(smem, smemWidth, x_odd == xodd, y_odd == yodd);
    rgb *= gains.xyz;

    if(toGray)
    {
        float gray = dot(rgb, greyscale);
        write_imagef(dst, gid, (float4)(gray));
    }
    else
    {
        write_imagef(dst, gid, (float4)(rgb, 1.0f));
    }
}

template __attribute__((mangled_name(convert_rg2rgb)))
__kernel void bayerKernel<false, false, false>(__read_only image2d_t src,
                                               __write_only image2d_t dst,
                                               float3 gains);
template __attribute__((mangled_name(convert_gb2rgb)))
__kernel void bayerKernel<false, true, false>(__read_only image2d_t src,
                                              __write_only image2d_t dst,
                                              float3 gains);
template __attribute__((mangled_name(convert_gr2rgb)))
__kernel void bayerKernel<true, false, false>(__read_only image2d_t src,
                                              __write_only image2d_t dst,
                                              float3 gains);
template __attribute__((mangled_name(convert_bg2rgb)))
__kernel void bayerKernel<true, true, false>(__read_only image2d_t src,
                                             __write_only image2d_t dst,
                                             float3 gains);
template __attribute__((mangled_name(convert_rg2gray)))
__kernel void bayerKernel<false, false, true>(__read_only image2d_t src,
                                              __write_only image2d_t dst,
                                              float3 gains);
template __attribute__((mangled_name(convert_gb2gray)))
__kernel void bayerKernel<false, true, true>(__read_only image2d_t src,
                                             __write_only image2d_t dst,
                                             float3 gains);
template __attribute__((mangled_name(convert_gr2gray)))
__kernel void bayerKernel<true, false, true>(__read_only image2d_t src,
                                             __write_only image2d_t dst,
                                             float3 gains);
template __attribute__((mangled_name(convert_bg2gray)))
__kernel void bayerKernel<true, true, true>(__read_only image2d_t src,
                                            __write_only image2d_t dst,
                                            float3 gains);

template __attribute__((mangled_name(convert_rg2rgb_local)))
__kernel void bayerKernel_local<false, false, false>(__read_only image2d_t src,
                                                     __write_only image2d_t dst,
                                                     float3 gains,
                                                     __local float* smem,
                                                     int smemWidth, int smemHeight);
template __attribute__((mangled_name(convert_gb2rgb_local)))
__kernel void bayerKernel_local<false, true, false>(__read_only image2d_t src,
                                                    __write_only image2d_t dst,
                                                    float3 gains,
                                                    __local float* smem,
                                                    int smemWidth, int smemHeight);
template __attribute__((mangled_name(convert_gr2rgb_local)))
__kernel void bayerKernel_local<true, false, false>(__read_only image2d_t src,
                                                    __write_only image2d_t dst,
                                                    float3 gains,
                                                    __local float* smem,
                                                    int smemWidth, int smemHeight);
template __attribute__((mangled_name(convert_bg2rgb_local)))
__kernel void bayerKernel_local<true, true, false>(__read_only image2d_t src,
                                                   __write_only image2d_t dst,
                                                   float3 gains,
                                                   __local float* smem,
                                                   int smemWidth, int smemHeight);
template __attribute__((mangled_name(convert_rg2gray_local)))
__kernel void bayerKernel_local<false, false, true>(__read_only image2d_t src,
                                                    __write_only image2d_t dst,
                                                    float3 gains,
                                                    __local float* smem,
                                                    int smemWidth, int smemHeight);
template __attribute__((mangled_name(convert_gb2gray_local)))
__kernel void bayerKernel_local<false, true, true>(__read_only image2d_t src,
                                                   __write_only image2d_t dst,
                                                   float3 gains,
                                                   __local float* smem,
                                                   int smemWidth, int smemHeight);
template __attribute__((mangled_name(convert_gr2gray_local)))
__kernel void bayerKernel_local<true, false, true>(__read_only image2d_t src,
                                                   __write_only image2d_t dst,
                                                   float3 gains,
                                                   __local float* smem,
                                                   int smemWidth, int smemHeight);
template __attribute__((mangled_name(convert_bg2gray_local)))
__kernel void bayerKernel_local<true, true, true>(__read_only image2d_t src,
                                                  __write_only image2d_t dst,
                                                  float3 gains,
                                                  __local float* smem,
                                                  int smemWidth, int smemHeight);
#else
    __attribute__((always_inline)) bool opTrue(bool o) { return o; }
    __attribute__((always_inline)) bool opFalse(bool o) { return !o; }

    #define DEFINE_BAYER_KERNEL(name, xo, yo, toGray) \
        __kernel void name(__read_only image2d_t src, \
                           __write_only image2d_t dst) \
        { \
            int2 gid = (int2) { get_global_id(0), get_global_id(1) }; \
            int2 size = get_image_dim(src); \
            if(any(gid >= size)) \
                return; \
            bool x_odd = gid.x & 0x01; bool y_odd = gid.y & 0x01; \
            float3 rgb = convert_bayer2rgb(src, sampler, xo(x_odd), yo(y_odd)); \
            \
            if(toGray) \
            { \
                float gray = dot(rgb, greyscale); \
                write_imagef(dst, gid, (float4)(gray)); \
            } \
            else \
            { \
                write_imagef(dst, gid, (float4)(rgb, 1.0f)); \
            } \
        }
    #define DEFINE_BAYER_KERNEL_LOCAL(name, xo, yo, toGray) \
        __kernel void name##_local(__read_only image2d_t src, \
                                   __write_only image2d_t dst, \
                                   float3 gains, \
                                   __local float* smem, \
                                   int smemWidth, int smemHeight) \
        { \
            int2 gid = (int2) { get_global_id(0), get_global_id(1) }; \
            int2 lid = { get_local_id(0), get_local_id(1) }; \
            int2 size = get_image_dim(src); \
            if(any(gid >= size)) \
                return; \
            bool x_odd = gid.x & 0x01; bool y_odd = gid.y & 0x01; \
            \
            contextToLocalMemory_image2(src, sampler, 1, 1, smem, smemWidth, smemHeight); \
            barrier(CLK_LOCAL_MEM_FENCE); \
            \
            float3 rgb = convert_bayer2rgb_local(smem, smemWidth, xo(x_odd), yo(y_odd)); \
            rgb *= gains.xyz; \
            \
            if(toGray) \
            { \
                float gray = dot(rgb, greyscale); \
                write_imagef(dst, gid, (float4)(gray)); \
            } \
            else \
            { \
                write_imagef(dst, gid, (float4)(rgb, 1.0f)); \
            } \
        }

    DEFINE_BAYER_KERNEL(convert_rg2rgb,  opFalse, opFalse, false)
    DEFINE_BAYER_KERNEL(convert_gb2rgb,  opFalse, opTrue,  false)
    DEFINE_BAYER_KERNEL(convert_gr2rgb,  opTrue,  opFalse, false)
    DEFINE_BAYER_KERNEL(convert_bg2rgb,  opTrue,  opTrue,  false)
    DEFINE_BAYER_KERNEL(convert_rg2gray, opFalse, opFalse, true)
    DEFINE_BAYER_KERNEL(convert_gb2gray, opFalse, opTrue,  true)
    DEFINE_BAYER_KERNEL(convert_gr2gray, opTrue,  opFalse, true)
    DEFINE_BAYER_KERNEL(convert_bg2gray, opTrue,  opTrue,  true)
    DEFINE_BAYER_KERNEL_LOCAL(convert_rg2rgb,  opFalse, opFalse, false)
    DEFINE_BAYER_KERNEL_LOCAL(convert_gb2rgb,  opFalse, opTrue,  false)
    DEFINE_BAYER_KERNEL_LOCAL(convert_gr2rgb,  opTrue,  opFalse, false)
    DEFINE_BAYER_KERNEL_LOCAL(convert_bg2rgb,  opTrue,  opTrue,  false)
    DEFINE_BAYER_KERNEL_LOCAL(convert_rg2gray, opFalse, opFalse, true)
    DEFINE_BAYER_KERNEL_LOCAL(convert_gb2gray, opFalse, opTrue,  true)
    DEFINE_BAYER_KERNEL_LOCAL(convert_gr2gray, opTrue,  opFalse, true)
    DEFINE_BAYER_KERNEL_LOCAL(convert_bg2gray, opTrue,  opTrue,  true)
#endif