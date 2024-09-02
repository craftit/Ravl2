//
// Created by charles on 02/09/24.
//

#include "Ravl2/Dlib/Display.hh"

#include <cstdlib>
#include <spdlog/spdlog.h>
#include <optional>
#include "Ravl2/Dlib/Display.hh"
#include "Ravl2/Dlib/DisplayWindow.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2
{
  void initDLibDisplay()
  {}

  //! Get window
  std::shared_ptr<DLibIO::DisplayWindow> getDLibDisplayWindow(std::string name)
  {
    static std::mutex mutex;
    static std::map<std::string, std::shared_ptr<DLibIO::DisplayWindow>> windows;
    std::lock_guard lock(mutex);
    auto win = windows.find(name);
    if(win == windows.end())
    {
      win = windows.emplace(name, std::make_shared<DLibIO::DisplayWindow>()).first;
    }
    return win->second;
  }


#if 0
  void registerDLibAtExitWait()
  {
    // Wait for a key press before closing the window/program
    [[maybe_unused]] static int reg = []()
    {
      SPDLOG_INFO("Registering OpenCV wait for key press");
      if(std::atexit([]() { cv::waitKey(0); }) != 0)
      {
        SPDLOG_WARN("Failed to register OpenCV wait for key press");
      }
      return 0;
    }();
  }

  namespace
  {

    [[maybe_unused]] bool g_dispFmt1 = outputFormatMap().add(std::make_shared<OutputFormatCall>("OpenCV", "", "dlib", -1, [](const ProbeOutputContext &ctx) -> std::optional<StreamOutputPlan> {
      auto convChain = typeConverterMap().find(typeid(cv::Mat), ctx.m_sourceType);
      if(!convChain.has_value() && ctx.m_sourceType != typeid(cv::Mat)) {
        return std::nullopt;
      }
      auto strm = std::make_shared<StreamOutputCall<dlib::array2d<uint8_t>>>([filename = ctx.m_filename](const dlib::array2d<uint8_t> &img, std::streampos pos) -> std::streampos {
        (void)pos;
        //registerAtExitWait();
        getDLibDisplayWindow("OpenCV").queue([img](DLibIO::DisplayWindow &win) {
          win.display(img);
        });
        return 0;
      });
      if(ctx.m_sourceType == typeid(cv::Mat)) {
        return StreamOutputPlan {.mStream=strm, .mConversion={}, .mCost=1.0f};
      }
      return StreamOutputPlan {.mStream=strm, .mConversion=convChain.value(), .mCost=convChain.value().conversionLoss()};
    }));

  }// namespace
#endif


}// namespace Ravl2