# RAVL2,  Recognition And Vision Library 2

This version of Ravl2 is work in progress, implementations maybe incomplete and may change significantly.

This code is an updated version of [University of Surrey's Center for Vision Speech and Signal Processing](https://www.surrey.ac.uk/centre-vision-speech-signal-processing)'s 
open source computer vision library [RAVL library](https://www.surrey.ac.uk/centre-vision-speech-signal-processing/resources/open-source-c-library)

The [original library can be found on Source Forge](https://sourceforge.net/projects/ravl/)

There are still also some files not fully ported, and parts of the cmake
templates that are still generic.

## Goals:

 - Efficient interoperability with other vision libraries such as opencv
 - Port algorithms from RAVL to use new c++ idioms

## Supported platforms

 - Linux
 - MacOS 

A windows port should be possible, but is not currently supported.

## Building

Pre-requisites:

 - CMake 3.21 or higher
 - A C++20 compatible compiler
 - OpenCV 4.5 or higher
 - BLAS and LAPACK libraries

The code uses many other libraries, but if not present natively they will be downloaded and built as part of the build process.
See 'Dependencies.cmake' for a full list of dependencies. Some of the libraries used include:

 - cereal - For serialization
 - spdlog - For logging
 - xtensor - For multi-dimensional arrays
 - xtensor-blas - For linear algebra
 - catch2 - For unit tests
 - fmt - For formatting
 - nlohmann_json - For json parsing
 - CLI11 - For command line parsing

Project options can be found in 'ProjectOptions.cmake'.

The build itself should be straight forward:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```


## Related code

  - Much of the code comes via a cmake port of the original Ravl library: https://github.com/craftit/RavlX.git
  - Adapted from ccmath. the original code can be found at: https://github.com/adis300/ccmath
  - The cmake setup is based on this project template: https://github.com/cpp-best-practices/cmake_template

## Code structure

The code is organized into the following directories:

- cmake: Contains cmake configuration files
- data: Contains data files used by examples and tests. These are not intended to be installed with the library.
- examples: Contains example code
- scripts: Scripts for working with the project, notably autoport_ravl.sh for updating the code from the original Ravl library.
- share: Resources used by the library. 
- src/Ravl2: Contains the source code
  - 3D: Contains 3D geometry code
  - DLib: Contains interoperability code with DLib.
  - Geometry: Affine transforms, and geometric constructions
  - Image: Contains image processing code
    - Segmentation: Image segmentation, boundaries, and region growing
  - IO : Contains file handling code
  - Math: Contains common math, linear algebra, statistics, and optimization code
  - OpenCV: Interoperability with OpenCV
  - OpenGL: Interoperability with OpenGL
  - Pixel: Pixel and colour space handling.
  - Qt: Interoperability with Qt
- test: Contains unit tests
- 

The project is intended to be documents by doxygen.

## Licensing

The original Ravl code was released under LGPL. Any new code in this project
is released under the MIT license.

## Coding Conventions

There is a .clang-format file in the root directory that can be used to format the code.

### Naming

 * Scoped objects like classes and namespaces are named with CamelCase. 
 * Functions and variables start with lower case.
 * Types such as template arguments end with a 'T'
 * It is preferred that member variables are prefixed with 'm'

### Argument order

The output is the first argument(s).

### Image coordinates

The origin of the image is in the top left corner, and the first dimension is the vertical axis, the second is the horizontal axis.


## Library Design

### Common functions

 * fill(x,value) - fill x with value
 * copy(x,y) - copy y to x
 * clip(x,y) - Return an array x clipped to the range specified by y
 * clamp(x,y) - Return a value x limited to the range specified by y
 * view(x) - Return a view of x which can be used as a Ravl2 type, this view maybe reshaped without copying the data.
 * access(x) - Return a reference to the data in x.  Assigning to an access object has broadcast semantics.
 * clone(x) - Return a deep copy of x that maybe modified without affecting the original.
 * inverse(x) - Return the inverse, usually a std:optional<...> transform or matrix of x
 * fit(&x,y) - Fit a model x to data y

### Array<x> 

 * The last dimension is contiguous in memory, allowing for efficient access to the data.
 * The array is a view of the data, and can be reshaped without copying the data.

These are designed to be easy to construct from other containers.

 * This uses a thread safe reference-counted handle to the data. 
 * Other than the buffer the uses are responsible for coordinating access to the data.

