## Features

Mouve is a node-based prototyping tool allowing you to rapidly create simple and complex computer vision algorithms from a set of nodes. Connected, they define an image data processing pipeline. Mouve let you test and tune your resultive pipeline in a real-time without the need to (re)compile any source file.

Nodes database consist of:

- input/output nodes for reading source image/video files and writing back the results,
- camera input nodes for JAI (should work with any GigE Vision compatible cameras) and USB Video Class (UVC) camera, 
- image filters, segmentation nodes and simple geometrical transformators,
- visualizing non-image data nodes,
- image features detectors and descriptors, as well as matchers,
- stateful nodes for video data,
- OpenCL based nodes for GPU-accelerated processing and analyzing of image data,
- auxillary nodes for OpenCL - data transfer

The vast majority of nodes are just thin wrappers for OpenCV algorithms.

Computer vision algorithms are not just about images but also about data extracted from them - such as image keypoints. Consequently, mouve defines couple of node data types: images (gray, rgb, mono, any), keypoints, generic (1 or 2d) arrays, matching results and OpenCL device-located data.

## Dependencies

Mouve depends on:

- CMake 3.9+
- OpenCV 2.4.x,
- Boost 1.55+,
- Qt5 (mainly for UI part),
- Intel Threading Building Blocks (Optional),
- JAI SDK (Optional for JAI's GigE cameras support)

## Building

Successfully built using MS Visual Studio 2017 on Windows 7 and GCC 5.4 on Ubuntu 16.04 LTS.

### Linux

```
mkdir _build
cd _build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
cmake --build .
```

### Windows

```
mkdir _build
cd _build
cmake ..
cmake --build . --config RelWithDebInfo
```

Most likely you'd have to manually provide a path to Boost, OpenCV and Qt5 libraries because on Windows there aren't any meaningful defaults.

## Screenshot

![Screenshot](ss.png?raw=true)

## License

Mouve is licensed under MIT license, see [COPYING](COPYING) file for details.
