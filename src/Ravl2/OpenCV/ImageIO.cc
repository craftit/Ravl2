//
// Created by charles galambos on 25/08/2024.
//

#include <opencv4/opencv2/opencv.hpp>
#include "Ravl2/OpenCV/Image.hh"
#include "Ravl2/OpenCV/ImageIO.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/InputStreamMem.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"

namespace Ravl2
{

  namespace {
    template<typename ViaPixelT,unsigned N=2>
    std::optional<StreamInputPlan> makePlan(cv::Mat img, const ProbeInputContext &ctx)
    {
      using ViaT = Ravl2::Array<ViaPixelT,N>;
      std::optional<ConversionChain> convChain = typeConverterMap().find(ctx.m_targetType, typeid(ViaT));
      if(!convChain.has_value())
        return std::nullopt;
      return StreamInputPlan {std::make_shared<InputStreamMem<ViaT> >(toArray<ViaPixelT,N>(img)), convChain.value(), convChain.value().conversionLoss()};
    }

    //std::function<std::optional<StreamInputPlan>(const ProbeInputContext &)/
    [[maybe_unused]] bool g_regFmt =inputFormatMap().add(std::make_shared<InputFormatCall>("OpenCV","png,jpg,jpeg,bmp,tiff",-1,[](const ProbeInputContext &ctx) -> std::optional<StreamInputPlan> {
      //! If we are looking for a cv::Mat, we can just read the file directly.
      if(ctx.m_targetType == typeid(cv::Mat)) {
        auto strm = std::make_shared<StreamInputCall<cv::Mat>>([filename = ctx.m_filename](std::streampos &pos) -> std::optional<cv::Mat> {
          if(pos != 0)
            return std::nullopt;
          return cv::imread(filename, cv::IMREAD_UNCHANGED);
        });
      }

        // The best we can do is loaded it directly and look for a conversion.
        cv::Mat img = cv::imread(ctx.m_filename, cv::IMREAD_UNCHANGED);
        if(img.empty())
          return std::nullopt;

        switch(img.type())
        {
          case CV_8UC1: { return makePlan<uint8_t>(img, ctx); }
          case CV_8SC1: { return makePlan<int8_t>(img, ctx); }
          case CV_16UC1: { return makePlan<uint16_t>(img, ctx); }
          case CV_16SC1: { return makePlan<int16_t>(img, ctx); }
          case CV_32SC1: { return makePlan<int32_t>(img, ctx); }
          case CV_32FC1: { return makePlan<float>(img, ctx); }
          case CV_64FC1: { return makePlan<double>(img, ctx); }
          default: { return std::nullopt; }
        }
        return std::nullopt;
    }));

    //std::function<std::optional<StreamOutputPlan>(const ProbeOutputContext &)/
    [[maybe_unused]] bool g_regFmt1 = outputFormatMap().add(std::make_shared<OutputFormatCall>("OpenCV","png,jpg,jpeg,bmp,tiff",-1,[](const ProbeOutputContext &ctx) -> std::optional<StreamOutputPlan> {

      //! If we are looking for a cv::Mat, we can just read the file directly.
      if(ctx.m_sourceType == typeid(cv::Mat)) {
#if 1
        auto strm = std::make_unique<StreamOutputCall<cv::Mat>>([filename = ctx.m_filename](const cv::Mat &img, std::streampos pos) -> std::streampos {
          if(pos != 0) {
            throw std::runtime_error("OpenCV output format does not support seeking.");
          }
          cv::imwrite(filename, img);
          return 0;
        });

        return StreamOutputPlan { std::move(strm), {}, 1.0f };
#endif
      }
      return std::nullopt;
    }));

    // Add some type conversion functions.
    // Not 1 as we lose range.

    [[maybe_unused]] bool g_reg = registerConversion([](Array<uint8_t,2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg1 = registerConversion([](Array<int8_t,2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg2 = registerConversion([](Array<uint16_t,2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg3 = registerConversion([](Array<int16_t,2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg4 = registerConversion([](Array<int32_t,2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg5 = registerConversion([](Array<float,2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg6 = registerConversion([](Array<double,2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);

  }
}