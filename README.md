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

## Building

### Windows 

Open (VS2012 or higher) solution file or QtCreator project file. In order to customize include and library paths you need to modify project property files (.props or .pri, depending on used IDE) located in Include directory. 

### Linux

Same as windows but only QtCreator file is applicable.

### Dependencies

Mouve depends on `OpenCV`, `boost` (`variant` module), `Qt` (mainly for UI part), `Intel tbb` and [`clw`](https://github.com/k0zmo/clw) which is a thin OpenCL wrapper.
OpenCL support is optionally and can be turned off by removing its project property file. Same thing concerns `Intel tbb` library. 

## Screenshot

![Screenshot](ss.png?raw=true)

## License

Mouve is licensed under MIT license, see [COPYING](COPYING) file for details.
