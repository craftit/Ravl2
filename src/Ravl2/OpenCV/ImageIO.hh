//
// Created by charles galambos on 25/08/2024.
//

#pragma once

namespace Ravl2
{
  //! Register the OpenCV image IO.
  //! File IO
  //!    This supports png,jpg,jpeg,bmp,tiff file formats.
  //!    - Limits at the time of writing, only supports 2D greyscale images.
  //! Video files, mp4, avi, etc.
  //     'file://path/to/video.mp4' - Open video file
  //! Video sources
  //!    'camera://1' - Open camera 1

  void initOpenCVImageIO();

}