//
// Created by charles on 25/08/24.
//

#pragma once

namespace Ravl2
{
  //! Enable OpenCV display file format.
  //! Displaying images.
  //!   This also supports displaying images via saving a file formated as follows: 'display://WindowName' filename.
  //!   If an image is save this way, the program will wait for a key press before exiting
  void initOpenCVDisplay();
  
  //! Wait for a key press
  void waitKey(int delay = 0);
  
}// namespace Ravl2