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

#if !defined(NMIXTURES)
#  error("NMIXTURES must be defined")
#endif

typedef struct mogParams
{
    float varThreshold;
    float backgroundRatio;
    float w0; // weight for a new mixture
    float var0; // variance for a new mixture
    float minVar; // lowest possible variance value
} mogParams_t;

__kernel void mog_image_unorm(__read_only image2d_t frame,
                              __write_only image2d_t dst,
                              __global float* mixtureData,
                              __constant mogParams_t* params,
                              const float alpha) // learning rate coefficient
{
    const int2 gid = { get_global_id(0), get_global_id(1) };
    const int2 size = get_image_dim(frame);
    
    if (!all(gid < size))
        return;
        
    sampler_t smp = CLK_NORMALIZED_COORDS_FALSE | 
        CLK_FILTER_NEAREST | 
        CLK_ADDRESS_CLAMP_TO_EDGE;
        
    float pix = read_imagef(frame, smp, gid).x * 255.0f;
    const int gid1 = gid.x + gid.y * size.x;
    const int size1 = size.x * size.y;
    int pdfMatched = -1;

    __private float weight[NMIXTURES];
    __private float mean[NMIXTURES];
    __private float var[NMIXTURES];
    __private float sortKey[NMIXTURES];

    #pragma unroll NMIXTURES
    for(int mx = 0; mx < NMIXTURES; ++mx)
    {
        weight[mx] = mixtureData[gid1 + size1 * (mx + 0 * NMIXTURES)];
        mean[mx]   = mixtureData[gid1 + size1 * (mx + 1 * NMIXTURES)];
        var[mx]    = mixtureData[gid1 + size1 * (mx + 2 * NMIXTURES)];

        // Because mixtures are already sorted (from previous frame)
        // we only need to check this until first match is found
        if(pdfMatched < 0)
        {
            float diff = pix - mean[mx];
            float d2 = diff*diff;
            float threshold = params->varThreshold * var[mx];
        
            // Same as:
            // if (diff > -2.5f * var && 
            //     diff < +2.5f * var)

            // Mahalanobis distance
            if(d2 < threshold)
                pdfMatched = mx;
        }
    }

    if(pdfMatched < 0)
    {
        // No matching mixture found - replace the weakest one
        pdfMatched = NMIXTURES - 1; 

        weight[pdfMatched] = params->w0;
        mean[pdfMatched] = pix;
        var[pdfMatched] = params->var0;
    }
    else
    {
        #pragma unroll NMIXTURES
        for(int mx = 0; mx < NMIXTURES; ++mx)
        {
            if(mx == pdfMatched)
            {
                float diff = pix - mean[mx];
#if ACCURATE_CALCULATIONS != 1
                weight[mx] = weight[mx] + alpha * (1 - weight[mx]);
                mean[mx] = mean[mx] + alpha * diff;
                var[mx] = max(params->minVar, var[mx] + alpha * (diff*diff - var[mx]));
#else
                #define PI_MULT_2 6.28318530717958647692f
                float rho = alpha / native_sqrt(PI_MULT_2 * var[mx]) * native_exp(-0.5f * diff*diff / var[mx]);

                weight[mx] = weight[mx] + alpha * (1 - weight[mx]);
                mean[mx] = mean[mx] + rho * diff;
                var[mx] = max(params->minVar, var[mx] + rho * (diff*diff - var[mx]));
#endif
            }
            else
            {
                // For the unmatched mixtures, mean and variance
                // are unchanged, only the weight is replaced by:
                // weight = (1 - alpha) * weight;

                weight[mx] = (1 - alpha) * weight[mx];
            }
        }
    }

    // Normalize weight and calculate sortKey
    float weightSum = 0.0f;
    #pragma unroll NMIXTURES
    for(int mx = 0; mx < NMIXTURES; ++mx)
        weightSum += weight[mx];

    float invSum = 1.0f / weightSum;
    #pragma unroll NMIXTURES
    for(int mx = 0; mx < NMIXTURES; ++mx)
    {
        weight[mx] *= invSum;
        sortKey[mx] = var[mx] > FLT_MIN
            ? weight[mx] / native_sqrt(var[mx])
            : 0;
    }

    // Sort mixtures (buble sort).
    // Every mixtures but the one with "completely new" weight and variance
    // are already sorted thus we need to reorder only that single mixture.
    for(int mx = pdfMatched-1; mx >= 0; --mx)
    {
        if(sortKey[mx] >= sortKey[mx+1])
            break;

        float tmp;
#define SWAP(x, y) tmp = x; x = y; y = tmp;
        SWAP(weight[mx], weight[mx+1]);
        SWAP(mean[mx], mean[mx+1]);
        SWAP(var[mx], var[mx+1]);
#undef SWAP
    }

    #pragma unroll NMIXTURES
    for(int mx = 0; mx < NMIXTURES; ++mx)
    {
        mixtureData[gid1 + size1 * (mx + 0 * NMIXTURES)] = weight[mx];
        mixtureData[gid1 + size1 * (mx + 1 * NMIXTURES)] = mean[mx];
        mixtureData[gid1 + size1 * (mx + 2 * NMIXTURES)] = var[mx];
    }

    // No match is found with any of the K Gaussians.
    // In this case, the pixel is classified as foreground
    if(pdfMatched < 0)
    {
        float pix = 1.0f;
        write_imagef(dst, gid, (float4) pix);
        return;
    }

    // If the Gaussian distribution is classified as a background one,
    // the pixel is classified as background,
    // otherwise pixel represents the foreground
    weightSum = 0.0f;
    for(int mx = 0; mx < NMIXTURES; ++mx)
    {
        // The first Gaussian distributions which exceed
        // certain threshold (backgroundRatio) are retained for 
        // a background distribution.

        // The other distributions are considered
        // to represent a foreground distribution
        weightSum += weight[mx];

        if(weightSum > params->backgroundRatio)
        {
            float pix = pdfMatched > mx 
                ? 1.0f // foreground
                : 0.0f;  // background
            write_imagef(dst, gid, (float4) pix);
            return;
        }
    }
}

__kernel void mog_background_image_unorm(__write_only image2d_t dst,
                                         __global float* mixtureData,
                                         __constant mogParams_t* params)
{
    const int2 gid = { get_global_id(0), get_global_id(1) };
    const int2 size = get_image_dim(dst); 
    
    if (!all(gid < size))
        return;
        
    const int gid1d = gid.x + gid.y * size.x;
    const int size1d = size.x * size.y;
        
    float meanVal = 0.0f;
    float totalWeight = 0.0f;
    
    for(int mx = 0; mx < NMIXTURES; ++mx)
    {
        float weight = mixtureData[gid1d + size1d * (mx + 0 * NMIXTURES)];
        float mean   = mixtureData[gid1d + size1d * (mx + 1 * NMIXTURES)];
        
        meanVal += weight * mean;
        totalWeight += weight;

        if(totalWeight > params->backgroundRatio)
            break;
    }
    
    meanVal = meanVal * (1.f / totalWeight);
    write_imagef(dst, gid, (float4)(meanVal / 255.0f));
}
