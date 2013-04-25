__kernel void fill_image_uint(__write_only image2d_t dst, uint value)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    if(all(gid < get_image_dim(dst)))
        write_imageui(dst, gid, (uint4)(value));
}

__kernel void fill_image_int(__write_only image2d_t dst, int value)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    if(all(gid < get_image_dim(dst)))
        write_imagei(dst, gid, (int4)(value));
}

__kernel void fill_image_norm(__write_only image2d_t dst, float value)
{
    int2 gid = { get_global_id(0), get_global_id(1) };
    if(all(gid < get_image_dim(dst)))
        write_imagef(dst, gid, (float4)(value));
}