
This directory contains image io for various formats:

 * libjpeg-turbo
 * libpng

These formats will load the images in a way that minimise the information lost
to the requested format.  This is important for applications that require high
quality image processing, such as medical imaging or computer vision.

These formats are also optimized for performance, making them suitable for
real-time applications and large-scale image processing tasks.

They support HDR images.


See JpegLoaderPlan.md in this directory for a detailed plan to implement a JPEG loader that integrates directly with Ravl2 Array and PlanarImage types while participating in the IO probing and conversion-loss selection mechanism.
