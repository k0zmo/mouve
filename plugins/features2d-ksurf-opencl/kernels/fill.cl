/*
 * Copyright (c) 2013-2018 Kajetan Swierk <k0zmo@outlook.com>
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

__kernel void fill_image_uint(__write_only image2d_t dst, uint value)
{
    int2 gid = {get_global_id(0), get_global_id(1)};
    if (all(gid < get_image_dim(dst)))
        write_imageui(dst, gid, (uint4)(value));
}

__kernel void fill_image_int(__write_only image2d_t dst, int value)
{
    int2 gid = {get_global_id(0), get_global_id(1)};
    if (all(gid < get_image_dim(dst)))
        write_imagei(dst, gid, (int4)(value));
}

__kernel void fill_image_norm(__write_only image2d_t dst, float value)
{
    int2 gid = {get_global_id(0), get_global_id(1)};
    if (all(gid < get_image_dim(dst)))
        write_imagef(dst, gid, (float4)(value));
}

__kernel void fill_buffer_int(__global int* dst, int value, int n)
{
    int gid = get_global_id(0);
    if (gid < n)
        dst[gid] = value;
}