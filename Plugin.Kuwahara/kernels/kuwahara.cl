__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;
__constant sampler_t samplerKernel = CLK_NORMALIZED_COORDS_TRUE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;

#define Q_COEFF 8.0f
#define ALPHA_COEFF 1.0f
#if !defined(N_SECTORS)
#  error("N_SECTORS must be defined")
#endif

__kernel void kuwahara(__read_only image2d_t src,
                       __write_only image2d_t dst,
                       const int radius)
{
    int gx = get_global_id(0);
    int gy = get_global_id(1);

    if(gx >= get_image_width(src) || gy >= get_image_height(src))
        return;

    __private float mean[4] = {0};
    __private float sqmean[4] = {0};

    // top-left
    for(int j = -radius; j <= 0; ++j)
    {
        for(int i = -radius; i <= 0; ++i)
        {
            float pix = read_imagef(src, sampler, (int2)(gx + i, gy + j)).x;
            mean[0] += pix;
            sqmean[0] += pix * pix;
        }
    }

    // top-right
    for(int j = -radius; j <= 0; ++j)
    {
        for(int i = 0; i <= radius; ++i)
        {
            float pix = read_imagef(src, sampler, (int2)(gx + i, gy + j)).x;
            mean[1] += pix;
            sqmean[1] += pix * pix;
        }
    }

    // bottom-right
    for(int j = 0; j <= radius; ++j)
    {
        for(int i = 0; i <= radius; ++i)
        {
            float pix = read_imagef(src, sampler, (int2)(gx + i, gy + j)).x;
            mean[2] += pix;
            sqmean[2] += pix * pix;
        }
    }

    // bottom-left
    for(int j = 0; j <= radius; ++j)
    {
        for(int i = -radius; i <= 0; ++i)
        {
            float pix = read_imagef(src, sampler, (int2)(gx + i, gy + j)).x;
            mean[3] += pix;
            sqmean[3] += pix * pix;
        }
    }

    float n = (float) ((radius + 1) * (radius + 1));
    float min_sigma = 1e6;

    for(int k = 0; k < 4; ++k)
    {
        mean[k] /= n;
        sqmean[k] = fabs(sqmean[k] / n -  mean[k]*mean[k]);
        if(sqmean[k] < min_sigma)
        {
            min_sigma = sqmean[k];
            write_imagef(dst, (int2)(gx, gy), (float4)(mean[k]));
        }
    }
}

__kernel void kuwaharaRgb(__read_only image2d_t src,
                          __write_only image2d_t dst,
                          const int radius)
{
    int gx = get_global_id(0);
    int gy = get_global_id(1);

    if(gx >= get_image_width(src) || gy >= get_image_height(src))
        return;

    __private float3 mean[4] = {0};
    __private float3 sqmean[4] = {0};

    // top-left
    for(int j = -radius; j <= 0; ++j)
    {
        for(int i = -radius; i <= 0; ++i)
        {
            float3 pix = read_imagef(src, sampler, (int2)(gx + i, gy + j)).xyz;
            mean[0] += pix;
            sqmean[0] += pix * pix;
        }
    }

    // top-right
    for(int j = -radius; j <= 0; ++j)
    {
        for(int i = 0; i <= radius; ++i)
        {
            float3 pix = read_imagef(src, sampler, (int2)(gx + i, gy + j)).xyz;
            mean[1] += pix;
            sqmean[1] += pix * pix;
        }
    }

    // bottom-right
    for(int j = 0; j <= radius; ++j)
    {
        for(int i = 0; i <= radius; ++i)
        {
            float3 pix = read_imagef(src, sampler, (int2)(gx + i, gy + j)).xyz;
            mean[2] += pix;
            sqmean[2] += pix * pix;
        }
    }

    // bottom-left
    for(int j = 0; j <= radius; ++j)
    {
        for(int i = -radius; i <= 0; ++i)
        {
            float3 pix = read_imagef(src, sampler, (int2)(gx + i, gy + j)).xyz;
            mean[3] += pix;
            sqmean[3] += pix * pix;
        }
    }

    float n = (float) ((radius + 1) * (radius + 1));
    float min_sigma = 1e6;

    for(int k = 0; k < 4; ++k)
    {
        mean[k] /= n;
        sqmean[k] = fabs(sqmean[k] / n -  mean[k]*mean[k]);
        // HSI model (Lightness is the average of the three components)
        float sigma = fabs(sqmean[k].x) + fabs(sqmean[k].y) + fabs(sqmean[k].z);

        if(sigma < min_sigma)
        {
            min_sigma = sigma;
            write_imagef(dst, (int2)(gx, gy), (float4)(mean[k], 1.0f));
        }
    }
}

#if N_SECTORS != 0

__kernel void generalizedKuwahara(__read_only image2d_t src,
                                  __write_only image2d_t dst,
                                  const int radius,
                                  __read_only image2d_t krnl)
{
    int2 gid = { get_global_id(0), get_global_id(1) };

    if(any(gid >= get_image_dim(src)))
        return;

    const float piN = 2.0f * M_PI_F / (float) N_SECTORS;
    const float cosPI = cos(piN);
    const float sinPI = sin(piN);

    __private float mean[N_SECTORS] = {0};
    __private float sqMean[N_SECTORS] = {0};
    __private float weight[N_SECTORS] = {0};

    for(int j = -radius; j <= radius; ++j)
    {
        for(int i = -radius; i <= radius; ++i)
        {
            // inside the radius x radius square but outside the target circle
            if(i*i + j*j > radius * radius)
                continue;

            // use half-identity circle coordinates
            float2 v = {i, j};
            v = (0.5f * v) / (float) radius;

            // sample image in circle neighbourhood
            // convert to 0 .. 255 range (better precision?)
            float c = read_imagef(src, sampler, gid + (int2){i, j}).x * 255.0f;

            #pragma unroll
            for(int k = 0; k < N_SECTORS; ++k)
            {
                float w = read_imagef(krnl, samplerKernel, (float2)(v + 0.5f)).x;

                weight[k] += w;
                mean[k]   += c * w;
                sqMean[k] += c * c * w;

                float v0 =  v.x*cosPI + v.y*sinPI;
                float v1 = -v.x*sinPI + v.y*cosPI;

                v.x = v0;
                v.y = v1;
            }
        }
    }

    float out = 0.0f;
    float sumWeight = 0.0f;

    #pragma unroll
    for (int k = 0; k < N_SECTORS; ++k)
    {
        mean[k] /= weight[k];
        sqMean[k] = sqMean[k] / weight[k] - mean[k] * mean[k];

        float w = 1.0f / (1.0f + pow(fabs(sqMean[k]), 0.5f * Q_COEFF));

        sumWeight += w;
        out += mean[k] * w;		
    }

    write_imagef(dst, gid, (float4)(out / sumWeight / 255.0f, 0.0, 0.0f, 1.0f));
}

__kernel void generalizedKuwaharaRgb(__read_only image2d_t src,
                                     __write_only image2d_t dst,
                                     const int radius,
                                     __read_only image2d_t krnl)
{
    int2 gid = { get_global_id(0), get_global_id(1) };

    if(any(gid >= get_image_dim(src)))
        return;

    const float piN = 2.0f * M_PI_F / (float) N_SECTORS;
    const float cosPI = cos(piN);
    const float sinPI = sin(piN);

    // packs mean (float3) and weight
    __private float4 mean[N_SECTORS] = {0};
    __private float3 sqMean[N_SECTORS] = {0};

    for(int j = -radius; j <= radius; ++j)
    {
        for(int i = -radius; i <= radius; ++i)
        {
            // inside the radius x radius square but outside the target circle
            if(i*i + j*j > radius * radius)
                continue;

            // use half-identity circle coordinates
            float2 v = {i, j};
            v = (0.5f * v) / (float) radius;

            // sample image in circle neighbourhood
            float3 c = read_imagef(src, sampler, gid + (int2){i, j}).xyz * 255.0f;

            #pragma unroll
            for(int k = 0; k < N_SECTORS; ++k)
            {
                float w = read_imagef(krnl, samplerKernel, (float2)(v + 0.5f)).x;

                mean[k]   += (float4)(c * w, w);
                sqMean[k] += c * c * w;

                float v0 =  v.x*cosPI + v.y*sinPI;
                float v1 = -v.x*sinPI + v.y*cosPI;

                v.x = v0;
                v.y = v1;
            }
        }
    }

    // packs out (float3) sumWeight
    float4 out = {0.0f, 0.0f, 0.0f, 0.0f};

    #pragma unroll
    for (int k = 0; k < N_SECTORS; ++k)
    {
        mean[k].xyz /= mean[k].w;
        sqMean[k] = fabs(sqMean[k] / mean[k].w - mean[k].xyz * mean[k].xyz);

        float sigmaSum = sqMean[k].x + sqMean[k].y + sqMean[k].z;
        float w = 1.0f / (1.0f + pow(sigmaSum, 0.5f * Q_COEFF));

        out += (float4)(mean[k].xyz * w, w);
    }

    write_imagef(dst, gid, (float4)(out.xyz / out.w / 255.0f, 1.0f));
}

__kernel void calcStructureTensor(__read_only image2d_t src,
                                  __write_only image2d_t smm)
{
    int2 gid = { get_global_id(0), get_global_id(1) };

    if(any(gid >= get_image_dim(src)))
        return;

    // Calculate derivative over x
    //     [-1 0 1]
    // H = [-2 0 2]
    //     [-1 0 1]
    float dx  = -1 * read_imagef(src, sampler, gid + (int2)(-1,-1)).x;
          dx +=  1 * read_imagef(src, sampler, gid + (int2)( 1,-1)).x;
          dx += -2 * read_imagef(src, sampler, gid + (int2)(-1, 0)).x;
          dx +=  2 * read_imagef(src, sampler, gid + (int2)( 1, 0)).x;
          dx += -1 * read_imagef(src, sampler, gid + (int2)(-1, 1)).x;
          dx +=  1 * read_imagef(src, sampler, gid + (int2)( 1, 1)).x;

    // Calculate derivative over y
    //     [-1 -2 -1]
    // H = [ 0  0  0]
    //     [ 1  2  1]
    float dy  = -1 * read_imagef(src, sampler, gid + (int2)(-1,-1)).x;
          dy += -2 * read_imagef(src, sampler, gid + (int2)( 0,-1)).x;
          dy += -1 * read_imagef(src, sampler, gid + (int2)( 1,-1)).x;
          dy +=  1 * read_imagef(src, sampler, gid + (int2)(-1, 1)).x;
          dy +=  2 * read_imagef(src, sampler, gid + (int2)( 0, 1)).x;
          dy +=  1 * read_imagef(src, sampler, gid + (int2)( 1, 1)).x;

    write_imagef(smm, gid, (float4)(dx*dx, dy*dy, dx*dy, 0.0f));
}

__kernel void calcStructureTensorRgb(__read_only image2d_t src,
                                     __write_only image2d_t smm)
{
    int2 gid = { get_global_id(0), get_global_id(1) };

    if(any(gid >= get_image_dim(src)))
        return;

    // Calculate derivative over x
    //     [-1 0 1]
    // H = [-2 0 2]
    //     [-1 0 1]
    float3 dx  = -1 * read_imagef(src, sampler, gid + (int2)(-1,-1)).xyz;
           dx +=  1 * read_imagef(src, sampler, gid + (int2)( 1,-1)).xyz;
           dx += -2 * read_imagef(src, sampler, gid + (int2)(-1, 0)).xyz;
           dx +=  2 * read_imagef(src, sampler, gid + (int2)( 1, 0)).xyz;
           dx += -1 * read_imagef(src, sampler, gid + (int2)(-1, 1)).xyz;
           dx +=  1 * read_imagef(src, sampler, gid + (int2)( 1, 1)).xyz;

    // Calculate derivative over y
    //     [-1 -2 -1]
    // H = [ 0  0  0]
    //     [ 1  2  1]
    float3 dy  = -1 * read_imagef(src, sampler, gid + (int2)(-1,-1)).xyz;
           dy += -2 * read_imagef(src, sampler, gid + (int2)( 0,-1)).xyz;
           dy += -1 * read_imagef(src, sampler, gid + (int2)( 1,-1)).xyz;
           dy +=  1 * read_imagef(src, sampler, gid + (int2)(-1, 1)).xyz;
           dy +=  2 * read_imagef(src, sampler, gid + (int2)( 0, 1)).xyz;
           dy +=  1 * read_imagef(src, sampler, gid + (int2)( 1, 1)).xyz;

    write_imagef(smm, gid, (float4)(dot(dx,dx), dot(dy,dy), dot(dx,dy), 0.0f));
}

__constant float gw[] = { 
    0.200565413f,
    0.176998362f,
    0.121649072f,
    0.0651140586f,
    0.0271435771f,
    0.00881222915f,
};

__kernel void convolutionGaussian(__read_only image2d_t src,
                                  __write_only image2d_t dst)
{
    int2 gid = { get_global_id(0), get_global_id(1) };

    if(any(gid >= get_image_dim(src)))
        return;

    float4 sum = {0.0f, 0.0f, 0.0f, 0.0f};

    #pragma unroll
    for(int y = -5; y <= 5; ++y)
    {
        for(int x = -5; x <= 5; ++x)
        {
            float4 c = read_imagef(src, sampler, gid + (int2)(x, y)).xyzw;
            sum += c * gw[abs(y)] * gw[abs(x)];
        }
    }

    write_imagef(dst, gid, sum);
}

__kernel void calcOrientationAndAnisotropy(__read_only image2d_t smm,
                                           __write_only image2d_t oriani)
{
    int2 gid = { get_global_id(0), get_global_id(1) };

    if(any(gid >= get_image_dim(smm)))
        return;

    float3 g = read_imagef(smm, sampler, gid).xyz;

    float a = g.x - g.y;
    float sqrt_ = sqrt(a*a + 4*g.z*g.z);
    float lambda1 = 0.5f * (g.x + g.y + sqrt_);
    float lambda2 = 0.5f * (g.x + g.y - sqrt_);

    float2 t = { lambda1 - g.x,
                -g.z };
    if(dot(t,t) > 0)
        t = normalize(t);
    else 
        t = (float2) { 0, 1.0f };
        
    float phi = atan2(t.y, t.x);
    float A = (lambda1 + lambda2 > 0) ? (lambda1 - lambda2) / (lambda1 + lambda2) : 0.0f;

    write_imagef(oriani, gid, (float4)(phi, A, 0, 0));
}

__kernel void anisotropicKuwahara(__read_only image2d_t src,
                                  __read_only image2d_t krnl,
                                  __read_only image2d_t oriAni,
                                  __write_only image2d_t dst,
                                  const int radius)
{
    int2 gid = { get_global_id(0), get_global_id(1) };

    if(any(gid >= get_image_dim(src)))
        return;

    const float piN = 2.0f * M_PI_F / (float) N_SECTORS;
    const float cosPI = cos(piN);
    const float sinPI = sin(piN);

    float2 t = read_imagef(oriAni, sampler, gid).xy;
    float a = radius * clamp((ALPHA_COEFF + t.y) / ALPHA_COEFF, 0.1f, 2.0f);
    float b = radius * clamp(ALPHA_COEFF / (ALPHA_COEFF + t.y), 0.1f, 2.0f);

    float cos_phi = cos(t.x);
    float sin_phi = sin(t.x);

    float4 sr = { 0.5f/a * cos_phi,
                  0.5f/a * sin_phi,
                 -0.5f/b * sin_phi,
                  0.5f/b * cos_phi };

    cos_phi *= cos_phi;
    sin_phi *= sin_phi;
    a *= a;
    b *= b;

    int max_x = (int) sqrt(a*cos_phi + b*sin_phi);
    int max_y = (int) sqrt(a*sin_phi + b*cos_phi);

    __private float mean[N_SECTORS] = {0};
    __private float sqMean[N_SECTORS] = {0};
    __private float weight[N_SECTORS] = {0};

    for(int j = -max_y; j <= max_y; ++j)
    {
        for(int i = -max_x; i <= max_x; ++i)
        {
            float2 v = { sr.x*i + sr.y*j, sr.z*i + sr.w*j };

            if(dot(v,v) < 0.25f)
            {
                // sample image in circle neighbourhood
                // convert to 0 .. 255 range (better precision?)
                float c = read_imagef(src, sampler, gid + (int2){i, j}).x * 255.0f;

                #pragma unroll
                for(int k = 0; k < N_SECTORS; ++k)
                {
                    float w = read_imagef(krnl, samplerKernel, (float2)(v + 0.5f)).x;

                    weight[k] += w;
                    mean[k]   += c * w;
                    sqMean[k] += c * c * w;

                    float v0 =  v.x*cosPI + v.y*sinPI;
                    float v1 = -v.x*sinPI + v.y*cosPI;

                    v.x = v0;
                    v.y = v1;
                }
            }
        }
    }

    float out = 0.0f;
    float sumWeight = 0.0f;

    #pragma unroll
    for (int k = 0; k < N_SECTORS; ++k)
    {
        mean[k] /= weight[k];
        sqMean[k] = sqMean[k] / weight[k] - mean[k] * mean[k];

        float w = 1.0f / (1.0f + pow(fabs(sqMean[k]), 0.5f * Q_COEFF));

        sumWeight += w;
        out += mean[k] * w;		
    }

    write_imagef(dst, gid, (float4)(out / sumWeight / 255.0f, 0.0, 0.0f, 1.0f));
}

__kernel void anisotropicKuwaharaRgb(__read_only image2d_t src,
                                     __read_only image2d_t krnl,
                                     __read_only image2d_t oriAni,
                                     __write_only image2d_t dst,
                                     const int radius)
{
    int2 gid = { get_global_id(0), get_global_id(1) };

    if(any(gid >= get_image_dim(src)))
        return;

    const float piN = 2.0f * M_PI_F / (float) N_SECTORS;
    const float cosPI = cos(piN);
    const float sinPI = sin(piN);

    float2 t = read_imagef(oriAni, sampler, gid).xy;
    float a = radius * clamp((ALPHA_COEFF + t.y) / ALPHA_COEFF, 0.1f, 2.0f);
    float b = radius * clamp(ALPHA_COEFF / (ALPHA_COEFF + t.y), 0.1f, 2.0f);

    float cos_phi = cos(t.x);
    float sin_phi = sin(t.x);

    float4 sr = { 0.5f/a * cos_phi,
                  0.5f/a * sin_phi,
                 -0.5f/b * sin_phi,
                  0.5f/b * cos_phi };

    cos_phi *= cos_phi;
    sin_phi *= sin_phi;
    a *= a;
    b *= b;

    int max_x = (int) sqrt(a*cos_phi + b*sin_phi);
    int max_y = (int) sqrt(a*sin_phi + b*cos_phi);

    // packs mean (float3) and weight
    __private float4 mean[N_SECTORS] = {0};
    __private float3 sqMean[N_SECTORS] = {0};

    for(int j = -max_y; j <= max_y; ++j)
    {
        for(int i = -max_x; i <= max_x; ++i)
        {
            float2 v = { sr.x*i + sr.y*j, sr.z*i + sr.w*j };

            if(dot(v,v) < 0.25f)
            {
                // sample image in circle neighbourhood
                // convert to 0 .. 255 range (better precision?)
                float3 c = read_imagef(src, sampler, gid + (int2){i, j}).xyz * 255.0f;

                #pragma unroll
                for(int k = 0; k < N_SECTORS; ++k)
                {
                    float w = read_imagef(krnl, samplerKernel, (float2)(v + 0.5f)).x;

                    mean[k]   += (float4)(c * w, w);
                    sqMean[k] += c * c * w;

                    float v0 =  v.x*cosPI + v.y*sinPI;
                    float v1 = -v.x*sinPI + v.y*cosPI;

                    v.x = v0;
                    v.y = v1;
                }
            }
        }
    }

    // packs out (float3) sumWeight
    float4 out = {0.0f, 0.0f, 0.0f, 0.0f};

    #pragma unroll
    for (int k = 0; k < N_SECTORS; ++k)
    {
        mean[k].xyz /= mean[k].w;
        sqMean[k] = fabs(sqMean[k] / mean[k].w - mean[k].xyz * mean[k].xyz);

        float sigmaSum = sqMean[k].x + sqMean[k].y + sqMean[k].z;
        float w = 1.0f / (1.0f + pow(sigmaSum, 0.5f * Q_COEFF));

        out += (float4)(mean[k].xyz * w, w);
    }

    write_imagef(dst, gid, (float4)(out.xyz / out.w / 255.0f, 1.0f));
}

#endif