//
// Created by charles on 02/09/24.
//

#include "Ravl2/Dlib/Display.hh"

#include <cstdlib>
#include <spdlog/spdlog.h>
#include <optional>
#include "Ravl2/Dlib/Image.hh"
#include "Ravl2/Dlib/DisplayWindow.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/Array.hh"

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
      win = windows.emplace(name, std::make_shared<DLibIO::DisplayWindow>(name)).first;
    }
    return win->second;
  }

  namespace
  {
  void registerDLibAtExitWait()
  {
    // Wait for a key press before closing the window/program
    [[maybe_unused]] static int reg = []()
    {
      SPDLOG_INFO("Registering OpenCV wait for key press");
      if(std::atexit([]() {
        SPDLOG_INFO("Waiting for key press");
        std::getchar();
         }) != 0)
      {
        SPDLOG_WARN("Failed to register OpenCV wait for key press");
      }
      return 0;
    }();
  }


    [[maybe_unused]] bool g_dispFmt1 = outputFormatMap().add(std::make_shared<OutputFormatCall>("OpenCV", "", "dlib", -1, [](const ProbeOutputContext &ctx) -> std::optional<StreamOutputPlan> {
      auto convChain = typeConverterMap().find(typeid(Ravl2::Array<uint8_t,2>), ctx.m_sourceType);
      if(ctx.m_protocol != "dlib") {
        return std::nullopt;
      }
      if(!convChain.has_value() && ctx.m_sourceType != typeid(Ravl2::Array<uint8_t,2>)) {
        return std::nullopt;
      }
      auto strm = std::make_shared<StreamOutputCall<Ravl2::Array<uint8_t,2>>>([filename = ctx.m_filename](const Ravl2::Array<uint8_t,2> &img, std::streampos pos) -> std::streampos {
        (void)pos;
        getDLibDisplayWindow(filename)->queue([img](DLibIO::DisplayWindow &win) {
          using namespace Ravl2::DLibIO;
          //DLibArray dimg(img);
          //win.display(dimg);
          win.display(toDlib(img));
        });
        registerDLibAtExitWait();
        return 0;
      });
      if(ctx.m_sourceType == typeid(Ravl2::Array<uint8_t,2>)) {
        return StreamOutputPlan {.mStream=strm, .mConversion={}, .mCost=1.0f};
      }
      return StreamOutputPlan {.mStream=strm, .mConversion=convChain.value(), .mCost=convChain.value().conversionLoss()};
    }));

  }// namespace




}// namespace Ravl2