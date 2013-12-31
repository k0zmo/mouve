__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_CLAMP_TO_EDGE;

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