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

#include "Node.cpp"
#include "NodeFactory.cpp"
#include "NodeFlowData.cpp"
#include "NodeLink.cpp"
#include "NodePlugin.cpp"
#include "NodeProperty.cpp"
#include "NodeSystem.cpp"
#include "NodeTree.cpp"
#include "NodeTreeSerializer.cpp"
#include "NodeType.cpp"

#include "Jai/JaiCameraNodes.cpp"
#include "Jai/JaiNodeModule.cpp"

#include "Nodes/ArithmeticNodes.cpp"
#include "Nodes/BriskNodes.cpp"
#include "Nodes/ColorConversionNodes.cpp"
#include "Nodes/CV.cpp"
#include "Nodes/DrawFeaturesNodes.cpp"
#include "Nodes/FeatureDetectorsNodes.cpp"
#include "Nodes/FeaturesNodes.cpp"
#include "Nodes/FiltersNodes.cpp"
#include "Nodes/FreakNodes.cpp"
#include "Nodes/HistogramNodes.cpp"
#include "Nodes/HomographyNodes.cpp"
#include "Nodes/KeypointsNodes.cpp"
#include "Nodes/ksurf.cpp"
#include "Nodes/kSurfNodes.cpp"
#include "Nodes/MatcherNodes.cpp"
#include "Nodes/MorphologyNodes.cpp"
#include "Nodes/MosaicingNodes.cpp"
#include "Nodes/OrbNodes.cpp"
#include "Nodes/SegmentationNodes.cpp"
#include "Nodes/SiftNodes.cpp"
#include "Nodes/SinkNodes.cpp"
#include "Nodes/SourceNodes.cpp"
#include "Nodes/SurfNodes.cpp"
#include "Nodes/TransformationNodes.cpp"
#include "Nodes/VideoSegmentationNodes.cpp"

#include "OpenCL/DeviceArray.cpp"
#include "OpenCL/GpuActivityLogger.cpp"
#include "OpenCL/GpuBasicNodes.cpp"
#include "OpenCL/GpuBruteForceMacherNode.cpp"
#include "OpenCL/GpuColorConversionNodes.cpp"
#include "OpenCL/GpuApproxGaussianBlur.cpp"
#include "OpenCL/GpuHoughLinesNode.cpp"
#include "OpenCL/GpuKernelLibrary.cpp"
#include "OpenCL/GpuMixtureOfGaussians.cpp"
#include "OpenCL/GpuMorphologyOperatorNode.cpp"
#include "OpenCL/GpuNodeModule.cpp"
#include "OpenCL/GpuSurfNode.cpp"