//
// Created by charles on 25/08/24.
//

#include <cstdlib>
#include "Ravl2/OpenCV/Display.hh"
#include <opencv4/opencv2/opencv.hpp>
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include <opencv2/highgui.hpp>

namespace Ravl2
{
  void initOpenCVDisplay()
  {}

  void registerAtExitWait()
  {
    // Wait for a key press before closing the window/program
    [[maybe_unused]] static int reg = std::atexit([]() {
      cv::waitKey(0);
    });
  }

  namespace
  {

    [[maybe_unused]] bool g_dispFmt1 = outputFormatMap().add(std::make_shared<OutputFormatCall>("OpenCV", "", "display", -1, [](const ProbeOutputContext &ctx) -> std::optional<StreamOutputPlan> {
      auto convChain = typeConverterMap().find(typeid(cv::Mat), ctx.m_sourceType);
      if(!convChain.has_value() && ctx.m_sourceType != typeid(cv::Mat))
        return std::nullopt;
      auto strm = std::make_shared<StreamOutputCall<cv::Mat>>([filename = ctx.m_filename](const cv::Mat &img, std::streampos pos) -> std::streampos {
        (void)pos;
        cv::imshow(filename, img);
        cv::pollKey();
        registerAtExitWait();
        return 0;
      });
      if(ctx.m_sourceType == typeid(cv::Mat)) {
        return StreamOutputPlan {strm, {}, 1.0f};
      }
      return StreamOutputPlan {strm, convChain.value(), convChain.value().conversionLoss()};
    }));

  }// namespace

}// namespace Ravl2