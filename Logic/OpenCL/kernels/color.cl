// Assumption: source is the same size (width x height) as destination
__kernel void convertBufferRgbToImageRgba(__global uchar* source,
                                          int sourcePitch,
                                          __write_only image2d_t destination)
{
    const int2 gid = { get_global_id(0), get_global_id(1) };
    const int2 size = get_image_dim(destination);

    if(!all(gid < size))
        return;

    int firstCoord = mad24(gid.y, sourcePitch, 3 * gid.x);

    float b = source[firstCoord + 0] * (1.0f / 255.0f);
    float g = source[firstCoord + 1] * (1.0f / 255.0f);
    float r = source[firstCoord + 2] * (1.0f / 255.0f);

    // source is BGR, destination is RGBX
    write_imagef(destination, gid, (float4)(r, g, b, 1.0f));
}

__constant sampler_t samp = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;

// Assumption: source is the same size (width x height) as destination
__kernel void convertImageRgbaToBufferRgb(__read_only image2d_t source,
                                          __global uchar* destination,
                                          int destinationPitch)
{
    const int2 gid = { get_global_id(0), get_global_id(1) };
    const int2 size = get_image_dim(source);

    if(!all(gid < size))
        return;

    float3 rgb = read_imagef(source, samp, gid).xyz;
    rgb *= (float3)(255.0f, 255.0f, 255.0f);


    int firstCoord = mad24(gid.y, destinationPitch, 3 * gid.x);

    // destination is BGR, source is RGBX
    destination[firstCoord + 0] = (uchar) rgb.z;
    destination[firstCoord + 1] = (uchar) rgb.y;
    destination[firstCoord + 2] = (uchar) rgb.x;
}