// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
//! author="Charles Galambos"

#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>

#include <internal_use_only/config.hh>

//#include "Ravl2/Image/CornerDetectorHarris.hh"
#include "Ravl2/Image/CornerDetectorSusan.hh"
#include "Ravl2/Image/DrawFrame.hh"
#include "Ravl2/Image/DrawCross.hh"
#include "Ravl2/OpenCV/convert.hh"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

int main(int argc,char **argv)
{
  using namespace Ravl2;

  CLI::App app{"App description"};

  int threshold = 30;
  int w = 3;
  bool useHarris = false;
  bool useTopHat = true;
  bool seq = false;
  bool verb = false;
  bool deinterlace = false;
  
  int frameLimit = 0;
  std::string inf = "/home/charles/src/Ravl2/data/lena.jpg";
  std::string outf = "out.ppm";

  app.add_option("-t", threshold, "Threshold. ");
  app.add_option("-w", w, "width of filter mask. ");
  app.add_flag("--hr", useHarris, "Use harris corner detector, else use susan. ");
  app.add_flag("--th", useTopHat, "Use top hat filter in harris corner detector. ");
  app.add_flag("--seq", seq, "Process a sequence. ");
  app.add_flag("-v", verb, "Verbose mode. ");
  app.add_flag("-d", deinterlace, "Deinterlace images");
  app.add_option("--fl", frameLimit, "Limit on the number of frames to process in a sequence. ");
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
  
//  CornerDetectorC cornerDet;
//  if(useHarris)
//    cornerDet = CornerDetectorHarrisC(threshold,w,useTopHat);
//  else
 auto cornerDet = Ravl2::CornerDetectorSusan(threshold);
  
  if(!seq) {
    // Process a signle image
    cv::startWindowThread();

    SPDLOG_INFO("Loading image '{}'", inf);

    cv::Mat cvImg = imread(inf, cv::IMREAD_GRAYSCALE);

    if(cvImg.data == nullptr) {
      SPDLOG_ERROR("Failed to load image '{}'", inf);
      return 1;
    }

    Ravl2::Array<uint8_t,2> img = Ravl2::toArray<uint8_t,2>(cvImg);



//    if(!RavlN::Load(inf,img)) { // Load an image.
//      SPDLOG_ERROR("Failed to load image '{}'", inf);
//      return 1;
//    }
    
    // Find the corners.
    
    std::vector<Ravl2::CornerC> corners;

    SPDLOG_INFO("Extracting corners");

    corners = cornerDet.Apply(img);

    SPDLOG_INFO("Drawing {} corners", corners.size());
    // Draw boxes around the corners.
    
    uint8_t val = 0;
    for(auto it : corners) {
      auto pixelIndex = Ravl2::toIndex<2>(it.Location());
      Ravl2::IndexRange<2> rect = Ravl2::IndexRange<2>(pixelIndex,pixelIndex).expand(5);
      SPDLOG_INFO("Drawing rect {} -> {}",  pixelIndex, rect);
      Ravl2::DrawFrame(img,val,rect);
      Ravl2::DrawCross(img, val, pixelIndex, 5);
    }
    
    // Save image to a file.
    SPDLOG_INFO("Displaying image");

    cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE );
    cv::imshow("Display Image", cvImg);
    SPDLOG_INFO("Press any key to exit.");
    cv::waitKey(0);

//    if(!RavlN::Save(outf,img)) {
//      cerr << "Failed to save image '" << inf << "'\n";
//      return 1;
//    }
    return 0;
  }
  SPDLOG_ERROR("Sequence processing not implemented yet.");
//  {
//    // Process a sequence
//
//    DPIPortC<ImageC<uint8_t> > imgIn;
//    if(!OpenISequence(imgIn,inf,"",verb)) {
//      cerr << "Failed to open input '" << inf << "' \n";
//      return 1;
//    }
//
//    DPOPortC<ImageC<uint8_t> > imgOut;
//    if(!outf.IsEmpty()) { // If there's no output specificied do nothing, (Good for profiling.)
//      if(!OpenOSequence(imgOut,outf,"",verb)) {
//        cerr << "Failed to open input '" << outf << "' \n";
//        return 1;
//      }
//    }
//
//    ImageC<uint8_t> img;
//    while(imgIn.Get(img) && frameLimit-- != 0) {
//      if(deinterlace)
//        img = DeinterlaceSubsample(img);
//
//      // Find the corners.
//
//      auto corners = cornerDet.Apply(img);
//
//      // Draw boxes around the corners.
//      if(imgOut.IsValid()) {
//        uint8_t val = 255;
//        for(auto it : corners) {
//          IndexRange<2> rect(it.Location(),5);
//          DrawFrame(img,val,rect);
//        }
//
//        // Write image out.
//
//        imgOut.Put(img);
//      }
//    }
//  }
  return 0;
}
