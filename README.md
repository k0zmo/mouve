# mouve - Computer vision system prototyping tool

Mouve allow you to create and test computer vision system from a set of connected nodes defining operations data flow.

Nodes database consist of:

- input/output nodes for reading source images/videos and writing results,
- image filters, segmentation nodes and simple geometrical transformators,
- visualizing non-image data nodes,
- image features detectors and descriptors, as well as matchers
- OpenCL based nodes from GPU-accelerated processing and analyzing of image data,
- auxillary nodes for OpenCL - data transfer

Most nodes are thin wrappers for OpenCV algorithms.

## Building

### Windows 

Open solution file (VS2012) or QtCreator project file. In order to customize include and library paths you need to modify project property files (.props or .pri, depending on used IDE) located in Include directory. 

### Linux

Same as windows but only QtCreator file is available.

### Dependencies

Mouve depends on `OpenCV`, `boost` (`variant` module), `Qt` (mainly for UI part), `Intel tbb` and [`clw`](https://github.com/k0zmo/clw) which is a thin OpenCL wrapper.
OpenCL support is optionally and can be turned off by removing its project property file. Same thing concerns `Intel tbb` library. 

## Screenshot

![Screenshot](ss.png?raw=true)

## License

mouve is licensed under MIT license, see [COPYING](COPYING) file for details.