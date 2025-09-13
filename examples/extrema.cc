// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"
//! docentry="Ravl.API.Images.Segmentation"

#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>

#include "Ravl2/Image/Segmentation/SegmentExtrema.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/config.hh"
#include "Ravl2/OpenCV/ImageIO.hh"
#include "Ravl2/Resource.hh"
#include "Ravl2/IO/OutputSequence.hh"
#include "Ravl2/IO/InputSequence.hh"

int main(int nargs,char **argv) {
  using RealT = float;

  Ravl2::initOpenCVImageIO();

  CLI::App app{"Corner detection example program"};

  int minSize = 10;
  int minMargin = 8;
  int limit = 255;
  bool seq = true;
  bool webCam = true;
  bool drawResults = true;
  bool drawBlack = false;
  bool invert = false;
  bool verbose = false;
  bool useMasks = false;
  std::string fn = Ravl2::findFileResource("data","lena.jpg",verbose);
  std::string ofn = "display://Boundaries";

  app.add_option("--ms", minSize, "Minimum region size. ");
  app.add_option("--mm", minMargin, "Minimum margin. ");
  app.add_option("-l", limit, "Limit on difference to consider.");
  app.add_flag("-s", seq, "Process a sequence. ");
  app.add_flag("--webcam", webCam, "Use webcam as input.");
  app.add_flag("-d", drawResults, "Draw results into a window.");
  app.add_flag("--db", drawBlack, "Draw results into a black background.");
  app.add_flag("--inv", invert, "Invert image before processing. ");
  app.add_flag("-v", verbose, "Verbose mode. ");
  app.add_flag("-m", useMasks, "Use masks. ");
  if(webCam) {
    fn = "camera://0";
  }
  if(drawResults) {
    ofn = "display://Extrema";
  }
  app.add_option("-i", fn, "Input image. ");
  app.add_option("-o", ofn, "Output boundaries. ");

  bool show_version = false;
  app.add_flag("--version", show_version, "Show version information");

  CLI11_PARSE(app, nargs, argv);

  if (show_version) {
    fmt::print("{}\n", Ravl2::cmake::project_version);
  }

  using ImageT = Ravl2::Array<uint8_t,2>;

  Ravl2::StreamOutputProxy<ImageT> outputStream  = Ravl2::openOutputStream<ImageT>(ofn);
  if(!outputStream.valid()) {
    return 1;
  }

  Ravl2::StreamInputProxy<ImageT> inputStream = Ravl2::openInputStream<ImageT>(fn);
  if(!inputStream.valid()) {
    return 1;
  }
  
  Ravl2::SegmentExtrema<uint8_t> lst(minSize,minMargin,limit);

  int numberOfFrames = 0;
  Ravl2::IndexRange<2> imgRange;
  std::chrono::steady_clock::duration totalTime {};
  while(true)
  {
    Ravl2::Array<uint8_t,2> img = inputStream.get();
    imgRange = img.range();
    Ravl2::Array<uint8_t,2> pimg;
    if(invert) {
      if(!pimg.range().contains(img.range())) {
        pimg = Ravl2::Array<uint8_t, 2>(img.range());
      }
      for(auto it = zip(pimg,img);it.valid();++it) {
        it.template data<0>() = 255 - it.template data<1>();
      }
    } else {
      pimg = img;
    }
    if(useMasks) {
      std::vector<Ravl2::Array<int,2> > masks = lst.applyMask(pimg);
      numberOfFrames++;
    } else {
      std::vector<Ravl2::Boundary> bounds;
      auto startTime = std::chrono::steady_clock::now();
      bounds = lst.apply(pimg);
      auto endTime = std::chrono::steady_clock::now();
      totalTime += (endTime-startTime);
      numberOfFrames++;
      if(verbose) {
        SPDLOG_INFO("Regions={}", bounds.size());
      }
      if(drawResults) {
        Ravl2::Array<uint8_t,2> res;
        if(!drawBlack)
          res = clone(img);
        else {
          res = Ravl2::Array<uint8_t,2>(img.range(), 0);
        }
        
        // Draw boundaries into image and display.
        for(auto bit : bounds) {
          for(auto it : bit.edges()) {
            res[it.leftPixel()] = 255;
          }
        }
        outputStream.put(res);
      }
    }
    if(!seq)
      break;
  }

  double timeSeconds = std::chrono::duration<double>(totalTime).count();
  SPDLOG_INFO("Frames a second {}", numberOfFrames/timeSeconds);
  SPDLOG_INFO("Pixels a second {}", (imgRange.area() * numberOfFrames)/timeSeconds);
  return 0;
}
