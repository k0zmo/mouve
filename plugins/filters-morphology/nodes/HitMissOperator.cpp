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

#include "Logic/NodeType.h"
#include "Logic/NodeFactory.h"

#include "Logic/Nodes/CV.h" // cvu::parellel_for

constexpr const int OBJ = 255;
constexpr const int BCK = 0;

namespace {
int skeletonZHLutTable[256] = {
    0, 0, 0, 1, 0, 0, 1, 3, 0, 0, 3, 1, 1, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 3, 0, 3, 3,
    0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 0, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 2, 0,
    0, 0, 3, 1, 0, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 3, 1, 3, 0, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 3, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 1, 0, 0, 0, 0, 2, 2, 0, 0, 2, 0, 0, 0};
}

class HitMissOperatorNodeType : public NodeType
{
public:
    HitMissOperatorNodeType() : _op(EHitMissOperation::Outline)
    {
        addInput("Source", ENodeFlowDataType::ImageMono);
        addOutput("Output", ENodeFlowDataType::ImageMono);
        addProperty("Operation type", _op)
            .setUiHints("item: Outline, item: Skeleton, item: Skeleton (Zhang-Suen)");
        setDescription("Performs hit-miss operation on a given image.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImageMono();

        // Validate inputs
        if (src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        switch (_op.cast<Enum>().cast<EHitMissOperation>())
        {
        case EHitMissOperation::Outline:
            hitmissOutline(src, dst);
            break;
        case EHitMissOperation::Skeleton:
            hitmissSkeleton(src, dst);
            break;
        case EHitMissOperation::SkeletonZhang:
            hitmissSkeletonZhangSuen(src, dst);
            break;
        }

        return ExecutionStatus(EStatus::Ok);
    }

private:
    void hitmissOutline(const cv::Mat& src, cv::Mat& dst)
    {
        dst = src.clone();

        // 1 - obiekt (bialy)
        // 0 - tlo (czarny)
        // X - dowolny

        // Element strukturalny
        // 1|1|1
        // 1|X|1
        // 1|1|1
        //

        const uchar* pixels2 = src.ptr<uchar>();
        uchar* pixels = dst.ptr<uchar>();
        int rowOffset = src.cols;

        cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range) {
            for (int y = range.start; y < range.end; ++y)
            {
                int offset = 1 + y * rowOffset;
                for (int x = 1; x < src.cols - 1; ++x)
                {
                    uchar v = pixels2[offset];

                    if (v == OBJ)
                    {
                        uchar p1 = pixels2[offset - rowOffset - 1];
                        uchar p2 = pixels2[offset - rowOffset];
                        uchar p3 = pixels2[offset - rowOffset + 1];
                        uchar p4 = pixels2[offset - 1];
                        uchar p6 = pixels2[offset + 1];
                        uchar p7 = pixels2[offset + rowOffset - 1];
                        uchar p8 = pixels2[offset + rowOffset];
                        uchar p9 = pixels2[offset + rowOffset + 1];

                        if (p1 == OBJ && p2 == OBJ && p3 == OBJ && p4 == OBJ && p6 == OBJ &&
                            p7 == OBJ && p8 == OBJ && p9 == OBJ)
                        {
                            v = BCK;
                        }
                    }

                    pixels[offset++] = v;
                }
            }
        });
    }

    inline int _skeleton_iter1(const cv::Mat& src, cv::Mat& dst)
    {
        // Element strukturalny pierwszy
        // 1|1|1
        // X|1|X
        // 0|0|0
        //

        int d = 0;

        cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range) {
            for (int y = range.start; y < range.end; ++y)
            {
                for (int x = 1; x < src.cols - 1; ++x)
                {
                    if (src.at<uchar>(y - 1, x - 1) == OBJ && src.at<uchar>(y - 1, x) == OBJ &&
                        src.at<uchar>(y - 1, x + 1) == OBJ && src.at<uchar>(y, x) == OBJ &&
                        src.at<uchar>(y + 1, x - 1) == BCK && src.at<uchar>(y + 1, x) == BCK &&
                        src.at<uchar>(y + 1, x + 1) == BCK)
                    {
                        dst.at<uchar>(y, x) = BCK;
                        d++;
                    }
                }
            }
        });

        return d;
    }

    inline int _skeleton_iter2(const cv::Mat& src, cv::Mat& dst)
    {
        // Element strukturalny pierwszy - 90 w lewo
        // 1|X|0
        // 1|1|0
        // 1|x|0
        //

        int d = 0;

        cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range) {
            for (int y = range.start; y < range.end; ++y)
            {
                for (int x = 1; x < src.cols - 1; ++x)
                {
                    if (src.at<uchar>(y - 1, x - 1) == OBJ && src.at<uchar>(y - 1, x + 1) == BCK &&
                        src.at<uchar>(y, x - 1) == OBJ && src.at<uchar>(y, x) == OBJ &&
                        src.at<uchar>(y, x + 1) == BCK && src.at<uchar>(y + 1, x - 1) == OBJ &&
                        src.at<uchar>(y + 1, x + 1) == BCK)
                    {
                        dst.at<uchar>(y, x) = BCK;
                        d++;
                    }
                }
            }
        });

        return d;
    }

    inline int _skeleton_iter3(const cv::Mat& src, cv::Mat& dst)
    {
        // Element strukturalny pierwszy - 180 w lewo
        // 0|0|0
        // X|1|X
        // 1|1|1
        //

        int d = 0;

        cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range) {
            for (int y = range.start; y < range.end; ++y)
            {
                for (int x = 1; x < src.cols - 1; ++x)
                {
                    if (src.at<uchar>(y - 1, x - 1) == BCK && src.at<uchar>(y - 1, x) == BCK &&
                        src.at<uchar>(y - 1, x + 1) == BCK && src.at<uchar>(y, x) == OBJ &&
                        src.at<uchar>(y + 1, x - 1) == OBJ && src.at<uchar>(y + 1, x) == OBJ &&
                        src.at<uchar>(y + 1, x + 1) == OBJ)
                    {
                        dst.at<uchar>(y, x) = BCK;
                        d++;
                    }
                }
            }
        });
        return d;
    }

    inline int _skeleton_iter4(const cv::Mat& src, cv::Mat& dst)
    {
        // Element strukturalny pierwszy - 270 w lewo
        // 0|X|1
        // 0|1|1
        // 0|X|1
        //

        int d = 0;

        cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range) {
            for (int y = range.start; y < range.end; ++y)
            {
                for (int x = 1; x < src.cols - 1; ++x)
                {
                    if (src.at<uchar>(y - 1, x - 1) == BCK && src.at<uchar>(y - 1, x + 1) == OBJ &&
                        src.at<uchar>(y, x - 1) == BCK && src.at<uchar>(y, x) == OBJ &&
                        src.at<uchar>(y, x + 1) == OBJ && src.at<uchar>(y + 1, x - 1) == BCK &&
                        src.at<uchar>(y + 1, x + 1) == OBJ)
                    {
                        dst.at<uchar>(y, x) = BCK;
                        d++;
                    }
                }
            }
        });

        return d;
    }

    inline int _skeleton_iter5(const cv::Mat& src, cv::Mat& dst)
    {
        // Element strukturalny drugi
        // X|1|X
        // 0|1|1
        // 0|0|X
        //

        int d = 0;

        cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range) {
            for (int y = range.start; y < range.end; ++y)
            {
                for (int x = 1; x < src.cols - 1; ++x)
                {
                    if (src.at<uchar>(y - 1, x) == OBJ && src.at<uchar>(y, x - 1) == BCK &&
                        src.at<uchar>(y, x) == OBJ && src.at<uchar>(y, x + 1) == OBJ &&
                        src.at<uchar>(y + 1, x - 1) == BCK && src.at<uchar>(y + 1, x) == BCK)
                    {
                        dst.at<uchar>(y, x) = BCK;
                        d++;
                    }
                }
            }
        });

        return d;
    }

    inline int _skeleton_iter6(const cv::Mat& src, cv::Mat& dst)
    {
        // Element strukturalny drugi - 90 stopni w lewo
        // X|1|X
        // 1|1|0
        // X|0|0
        //

        int d = 0;

        cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range) {
            for (int y = range.start; y < range.end; ++y)
            {
                for (int x = 1; x < src.cols - 1; ++x)
                {
                    if (src.at<uchar>(y - 1, x) == OBJ && src.at<uchar>(y, x - 1) == OBJ &&
                        src.at<uchar>(y, x) == OBJ && src.at<uchar>(y, x + 1) == BCK &&
                        src.at<uchar>(y + 1, x) == BCK && src.at<uchar>(y + 1, x + 1) == BCK)
                    {
                        dst.at<uchar>(y, x) = BCK;
                        d++;
                    }
                }
            }
        });

        return d;
    }

    inline int _skeleton_iter7(const cv::Mat& src, cv::Mat& dst)
    {
        // Element strukturalny drugi - 180 stopni w lewo
        // X|0|0
        // 1|1|0
        // X|1|X
        //

        int d = 0;

        cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range) {
            for (int y = range.start; y < range.end; ++y)
            {
                for (int x = 1; x < src.cols - 1; ++x)
                {
                    if (src.at<uchar>(y - 1, x) == BCK && src.at<uchar>(y - 1, x + 1) == BCK &&
                        src.at<uchar>(y, x - 1) == OBJ && src.at<uchar>(y, x) == OBJ &&
                        src.at<uchar>(y, x + 1) == BCK && src.at<uchar>(y + 1, x) == OBJ)
                    {
                        dst.at<uchar>(y, x) = BCK;
                        d++;
                    }
                }
            }
        });

        return d;
    }

    inline int _skeleton_iter8(const cv::Mat& src, cv::Mat& dst)
    {
        // Element strukturalny drugi - 270 stopni w lewo
        // 0|0|X
        // 0|1|1
        // X|1|X
        //

        int d = 0;

        cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range) {
            for (int y = range.start; y < range.end; ++y)
            {
                for (int x = 1; x < src.cols - 1; ++x)
                {
                    if (src.at<uchar>(y - 1, x - 1) == BCK && src.at<uchar>(y - 1, x) == BCK &&
                        src.at<uchar>(y, x - 1) == BCK && src.at<uchar>(y, x) == OBJ &&
                        src.at<uchar>(y, x + 1) == OBJ && src.at<uchar>(y + 1, x) == OBJ)
                    {
                        dst.at<uchar>(y, x) = BCK;
                        d++;
                    }
                }
            }
        });

        return d;
    }

    int hitmissSkeleton(const cv::Mat& _src, cv::Mat& dst)
    {
        int niters = 0;

        cv::Mat src = _src.clone();
        dst = src.clone();

        while (true)
        {
            // iteracja
            ++niters;
            int d = 0;

            d += _skeleton_iter1(src, dst);
            dst.copyTo(src);

            d += _skeleton_iter2(src, dst);
            dst.copyTo(src);

            d += _skeleton_iter3(src, dst);
            dst.copyTo(src);

            d += _skeleton_iter4(src, dst);
            dst.copyTo(src);

            d += _skeleton_iter5(src, dst);
            dst.copyTo(src);

            d += _skeleton_iter6(src, dst);
            dst.copyTo(src);

            d += _skeleton_iter7(src, dst);
            dst.copyTo(src);

            d += _skeleton_iter8(src, dst);

            if (d == 0)
                break;

            dst.copyTo(src);
        }

        return niters;
    }

    int hitmissSkeletonZhangSuen_pass(cv::Mat& dst, int pass)
    {
        const int bgColor = 0;

        cv::Mat tmp = dst.clone();
        uchar* pixels2 = tmp.ptr<uchar>();
        uchar* pixels = dst.ptr<uchar>();
        int pixelsRemoved = 0;
        int xMin = 1, yMin = 1, rowOffset = dst.cols;

        cvu::parallel_for(cv::Range(yMin, dst.rows - 1), [&](const cv::Range& range) {
            for (int y = range.start; y < range.end; ++y)
            {
                int offset = xMin + y * rowOffset;
                for (int x = xMin; x < dst.cols - 1; ++x)
                {
                    uchar p5 = pixels2[offset];
                    uchar v = p5;
                    if (v != bgColor)
                    {
                        uchar p1 = pixels2[offset - rowOffset - 1];
                        uchar p2 = pixels2[offset - rowOffset];
                        uchar p3 = pixels2[offset - rowOffset + 1];
                        uchar p4 = pixels2[offset - 1];
                        uchar p6 = pixels2[offset + 1];
                        uchar p7 = pixels2[offset + rowOffset - 1];
                        uchar p8 = pixels2[offset + rowOffset];
                        uchar p9 = pixels2[offset + rowOffset + 1];

                        // lut index
                        int index = ((p4 & 0x01) << 7) | ((p7 & 0x01) << 6) | ((p8 & 0x01) << 5) |
                                    ((p9 & 0x01) << 4) | ((p6 & 0x01) << 3) | ((p3 & 0x01) << 2) |
                                    ((p2 & 0x01) << 1) | (p1 & 0x01);
                        int code = skeletonZHLutTable[index];

                        // odd pass
                        if ((pass & 1) == 1)
                        {
                            if (code == 2 || code == 3)
                            {
                                v = bgColor;
                                pixelsRemoved++;
                            }
                        }
                        // even pass
                        else
                        {
                            if (code == 1 || code == 3)
                            {
                                v = bgColor;
                                pixelsRemoved++;
                            }
                        }
                    }
                    pixels[offset++] = v;
                }
            }
        });

        return pixelsRemoved;
    }

    int hitmissSkeletonZhangSuen(const cv::Mat& src, cv::Mat& dst)
    {
        // Based on ImageJ implementation of skeletonization which is
        // based on an a thinning algorithm by by Zhang and Suen (CACM, March 1984, 236-239)
        int pass = 0;
        int pixelsRemoved = 0;
        dst = src.clone();
        int niters = 0;

        do
        {
            niters++;
            pixelsRemoved = hitmissSkeletonZhangSuen_pass(dst, pass++);
            pixelsRemoved += hitmissSkeletonZhangSuen_pass(dst, pass++);
        } while (pixelsRemoved > 0);

        return niters;
    }

private:
    enum class EHitMissOperation
    {
        Outline,
        Skeleton,
        SkeletonZhang
    };

    TypedNodeProperty<EHitMissOperation> _op;
};

REGISTER_NODE("Morphology/Hit-miss", HitMissOperatorNodeType);
