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
#include "Ravl2/OpenCV/Image.hh"
#include "Ravl2/Resource.hh"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>

int main(int nargs,char **argv) {
  using namespace Ravl2;
  using RealT = float;
  
  CLI::App app{"Corner detection example program"};

  int minSize = 10;
  int minMargin = 10;
  int limit = 255;
  bool seq = false;
  bool webCam = true;
  bool drawResults = true;
  bool drawBlack = false;
  bool invert = false;
  bool verbose = false;
  bool useMasks = false;
  std::string fn = findFileResource("data","lena.jpg",verbose);
  std::string ofn = "";

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
  app.add_option("-i", fn, "Input image. ");
  app.add_option("-o", ofn, "Output boundaries. ");

  bool show_version = false;
  app.add_flag("--version", show_version, "Show version information");

  CLI11_PARSE(app, nargs, argv);

  if (show_version) {
    fmt::print("{}\n", Ravl2::cmake::project_version);
  }
  cv::startWindowThread();

  Ravl2::Array<uint8_t,2> img;
  
  SegmentExtremaC<uint8_t> lst(minSize,minMargin,limit);

  cv::VideoCapture videoCapture;

  if(seq) {
    if(webCam) {
      videoCapture.open(0);
    } else {
      videoCapture.open(fn);
    }
    if(!videoCapture.isOpened()) {
      SPDLOG_ERROR("Failed to open video stream '{}'", fn);
      return 1;
    }
  }
//  IndexRange2dSetC trimSet;
//  trimSet = trimSet.Add(IndexRange2dC(trim,trim));
  
  Ravl2::Array<uint8_t,2> pimg;
  int numberOfFrames = 0;
  std::chrono::steady_clock::duration totalTime {};
  while(true)
  {
    cv::Mat frame;
    if(seq) {
      videoCapture >> frame;
      if(frame.empty()) {
        SPDLOG_INFO("End of video stream");
        break;
      }
    } else {
      frame = cv::imread(fn, cv::IMREAD_GRAYSCALE);
      if(frame.empty()) {
        SPDLOG_ERROR("Failed to load image '{}'", fn);
        return 1;
      }
    }
    img = toArray<uint8_t, 2>(frame);

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
      std::vector<Boundary> bounds;
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
        {
          cv::Mat cvImg = toCvMat(res);
          cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE );
          cv::imshow("Display Image", cvImg);
        }
      }
    }
    if(!seq)
      break;
  }
  SPDLOG_INFO("Press any key to exit.");
  if(drawResults) {
    cv::waitKey(0);
  }

  double timeSeconds = std::chrono::duration<double>(totalTime).count();
  SPDLOG_INFO("Frames a second {}", numberOfFrames/timeSeconds);
  SPDLOG_INFO("Pixels a second {}", (img.range().area() * numberOfFrames)/timeSeconds);
  return 0;
}
