__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;
__constant sampler_t samplerKernel = CLK_NORMALIZED_COORDS_TRUE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;

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

#define Q_COEFF 8.0f
#define MAX_KERNEL_SECTORS 8

__kernel void generalizedKuwahara(__read_only image2d_t src,
                                  __write_only image2d_t dst,
                                  const int radius,
                                  __read_only image2d_t krnl,
                                  const int N)
{
    int2 gid = { get_global_id(0), get_global_id(1) };

    if(any(gid >= get_image_dim(src)))
        return;

    const float piN = 2.0f * M_PI_F / (float) N;
    const float cosPI = cos(piN);
    const float sinPI = sin(piN);

    __private float mean[MAX_KERNEL_SECTORS] = {0};
    __private float sqMean[MAX_KERNEL_SECTORS] = {0};
    __private float weight[MAX_KERNEL_SECTORS] = {0};

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

            for(int k = 0; k < N; ++k)
            {
                float w = read_imagef(krnl, samplerKernel, (float2)(v + 0.5f)).x;

                weight[k] += w;
                mean[k]   += c * w;
                sqMean[k] += c * c * w;

                float v0 =   v.x*cosPI + v.y*sinPI;
                float v1 =  -v.x*sinPI + v.y*cosPI;

                v.x = v0;
                v.y = v1;
            }
        }
    }

    float out = 0.0f;
    float sumWeight = 0.0f;

    for (int k = 0; k < N; ++k)
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
                                     __read_only image2d_t krnl,
                                     const int N)
{
    int2 gid = { get_global_id(0), get_global_id(1) };

    if(any(gid >= get_image_dim(src)))
        return;

    const float piN = 2.0f * M_PI_F / (float) N;
    const float cosPI = cos(piN);
    const float sinPI = sin(piN);

    // packs mean (float3) and weight
    __private float4 mean[MAX_KERNEL_SECTORS] = {0};
    __private float3 sqMean[MAX_KERNEL_SECTORS] = {0};

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

            for(int k = 0; k < N; ++k)
            {
                float w = read_imagef(krnl, samplerKernel, (float2)(v + 0.5f)).x;

                mean[k]   += (float4)(c * w, w);
                sqMean[k] += c * c * w;

                float v0 =   v.x*cosPI + v.y*sinPI;
                float v1 =  -v.x*sinPI + v.y*cosPI;

                v.x = v0;
                v.y = v1;
            }
        }
    }

    // packs out (float3) sumWeight
    float4 out = {0.0f, 0.0f, 0.0f, 0.0f};

    for (int k = 0; k < N; ++k)
    {
        mean[k].xyz /= mean[k].w;
        sqMean[k] = fabs(sqMean[k] / mean[k].w - mean[k].xyz * mean[k].xyz);

        float sigmaSum = sqMean[k].x + sqMean[k].y + sqMean[k].z;
        float w = 1.0f / (1.0f + pow(sigmaSum, 0.5f * Q_COEFF));

        out += (float4)(mean[k].xyz * w, w);
    }

    write_imagef(dst, gid, (float4)(out.xyz / out.w / 255.0f, 1.0f));
}
