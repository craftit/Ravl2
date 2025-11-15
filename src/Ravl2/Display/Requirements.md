
# Requirements for display

The goal of this c++ module is to allow 1d, 2d and 3d image processing to be debugged with minimal impact on the code.

The mechanism to interface to the library is the Ravl2::ioSave("display://[Name]",xyz). Where xyz maybe an image or other object. There is an automatic type converter mechanism that 
allows type conversion, which can convert the native types to render objects. These render objects can then manage interactions such as querying the pixel values.
When floating point data is saved the range can be normalised to that which is displayed, but the original values should be queueable.
It should be possible to zoom and pan around the 2d workspace. 

The 3d workspace it should be possible to move around in.  It should be possible to map the 2d renders into a 3d space if needed.

As a stretch goal the 3d space would support VR via a library like OpenHMD. 

 * Cross-platform, support Linux, Mac and Windows.
 * Display 2d and 3d data.
 * Use native Ravl2 classes where applicable.
 * Not too many dependencies, ideally as light weight as possible. 
 * Fast at update of images and render information.
 * Thread safe operations on the data to be displayed
 * Don't put a heavy load on the GPU when nothing is changing, dynamic framerate with some preset maximum ideally. 
 * Where possible should avoid requiring any special actions in the main() of the program.  It should setup any windows on the first ioSave(...)
 * It needs to be fast, there may be a large amount of data being displayed.
 * It should be possible to implement new display objects and extend the debug output with few if any changes to existing code.  This may involve writing a new rendering object and registering it with an appropriate type conversion.

Future features:

 * Dynamic graphs and waterfall plots.
 * Timelines and events.

# Use cases

  * I want to be able to render 2d objects such as images, then use the mouse point to query pixels values from the original image.
  * Handle multiple streams of data, such as images or 3d objects.
  * Optionally, support for ray tracing images to either offscreen or onscreen buffers.
  * It should also be possible to open Ravl2 streams and stream images to the display.

# Existing code

There is a simple image display based on OpenCV found at Ravl2/OpenCV/Display.cc, Ravl2/OpenCV/Display.hh.  We do not want to depend
on opencv.



# Example uses

The is the simplest use case:

```
#include "Ravl2/Array.hh"
#include "Ravl2/Point2d.hh"
#include "Ravl2/IO/Save.hh"
namespace Ravl2 {
  Array<float,2> edgemap;
  
  ... do some processing ...
  
  // Save the image we're processing to the display
  ioSave("display://EdgeMap",edgemap);
  
  std::vector<Point<float,2> > edges;

  ... some more processing ...
  
  // Add an overlay with the points we found
  ioSave("display://EdgeMap",edges);
  
}  
```

To replace the window contents you would call ioSave with the same name, but a hint to reset the display. This would then display
an animated sequence in the window.

```
#include "Ravl2/Array.hh"
#include "Ravl2/IO/Save.hh"
namespace Ravl2 {
    
  for(int i = 0;i < 10;i++) {
    Array<float,2> edgemap;
    
    ... do some processing ...
    
    ioSave("display://EdgeMap:Clear",edgemap);

    ... some more processing ...
    
    // Add an overlay with the points we found
    ioSave("display://EdgeMap",edges);
    
  }
}  
```

