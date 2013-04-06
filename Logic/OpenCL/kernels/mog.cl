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
    const int2 size = { get_image_width(frame), get_image_height(frame) };
    
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

        if(pdfMatched < 0)
        {
            float diff = pix - mean[mx];
            float d2 = diff*diff;
            float threshold = params->varThreshold * var[mx];
        
            // To samo co:
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

                #define PI_MULT_2 6.28318530717958647692f
                float rho = alpha / native_sqrt(PI_MULT_2 * var[mx]) * native_exp(-0.5f * diff*diff / var[mx]);

                weight[mx] = weight[mx] + alpha * (1 - weight[mx]);
                mean[mx] = mean[mx] + rho * diff;
                var[mx] = max(params->minVar, var[mx] + rho * (diff*diff - var[mx]));
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
    for(int mx = 0; mx < pdfMatched; ++mx)
    {
        if(sortKey[pdfMatched] > sortKey[mx])
        {
            float weightTemp = weight[pdfMatched];
            float meanTemp = mean[pdfMatched];
            float varTemp = var[pdfMatched];

            weight[pdfMatched] = weight[mx];
            mean[pdfMatched] = mean[mx];
            var[pdfMatched] = var[mx];

            weight[mx] = weightTemp;
            mean[mx] = meanTemp;
            var[mx] = varTemp;
            break;
        }
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
    const int2 size = { get_image_width(dst), get_image_height(dst) };
    
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