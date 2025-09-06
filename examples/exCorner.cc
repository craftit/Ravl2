// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>

#include "Ravl2/config.hh"
#include "Ravl2/Image/CornerDetectorSusan.hh"
#include "Ravl2/Image/DrawFrame.hh"
#include "Ravl2/OpenCV/ImageIO.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/Resource.hh"

int main(int argc,char **argv)
{
  Ravl2::initOpenCVImageIO();

  CLI::App app{"Corner detection example program"};

  int threshold = 30;
  bool verbose = false;

  app.add_option("-t", threshold, "Threshold. ");
  app.add_flag("-v", verbose, "Verbose mode. ");

  std::string inf = Ravl2::findFileResource("data","lena.jpg",verbose);
  std::string outf = "display://Corners";

  app.add_option("-i", inf, "Input image. ");
  app.add_option("-o", outf, "Output image. ");

  bool show_version = false;
  app.add_flag("--version", show_version, "Show version information");

  CLI11_PARSE(app, argc, argv);

  if (show_version) {
    fmt::print("{}\n", Ravl2::cmake::project_version);
    return EXIT_SUCCESS;
  }

  // Setup corner detector.
  
 auto cornerDet = Ravl2::CornerDetectorSusan(threshold);

  Ravl2::Array<uint8_t,2> img;

  if(!Ravl2::ioLoad(img, inf)) {
    SPDLOG_ERROR("Failed to load image '{}'", inf);
    return 1;
  }

  // Find the corners.
  std::vector<Ravl2::Corner> corners = cornerDet.apply(img);

  // Draw boxes around the corners.
  uint8_t val = 0;
  for(auto it : corners) {
    // Convert floating point to the closest integer index.
    auto pixelIndex = Ravl2::toIndex<2>(it.location());
    Ravl2::IndexRange<2> rect = Ravl2::IndexRange<2>(pixelIndex,pixelIndex).expand(5);
    Ravl2::DrawFrame(img,val,rect);
  }

  // Save image to a file.
  if(!Ravl2::ioSave(outf,img)) {
    SPDLOG_ERROR("Failed to save image");
    return 1;
  }

  return 0;
}
