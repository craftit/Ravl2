//
// Created by charles galambos on 25/08/2024.
//

#include <opencv4/opencv2/opencv.hpp>
#include "Ravl2/OpenCV/Image.hh"
#include "Ravl2/OpenCV/ImageIO.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"

namespace Ravl2
{

  namespace {
    //std::function<std::optional<InputPlanT>(const ProbeInputContext &)/
    [[maybe_unused]] bool g_regFmt = InputFormatMap().add(std::make_shared<InputFormatCall>("OpenCV","png,jpg,jpeg,bmp,tiff",-1,[](const ProbeInputContext &ctx) -> std::optional<InputFormat::InputPlanT> {
      //! If we are looking for a cv::Mat, we can just read the file directly.
      if(ctx.m_targetType == typeid(cv::Mat)) {
        auto strm = std::make_shared<StreamInputCall<cv::Mat>>([filename = ctx.m_filename](std::streampos &pos) -> std::optional<cv::Mat> {
          if(pos != 0)
            return std::nullopt;
          return cv::imread(filename, cv::IMREAD_UNCHANGED);
        });

        //std::tuple<std::shared_ptr<StreamInputBase>, ConversionChain> plan =
        return std::make_tuple(std::shared_ptr<StreamInputBase>(strm), ConversionChain());
      }

      return std::nullopt;
    }));

    //std::function<std::optional<OutputPlanT>(const ProbeOutputContext &)/
    [[maybe_unused]] bool g_regFmt1 = OutputFormatMap().add(std::make_shared<OutputFormatCall>("OpenCV","png,jpg,jpeg,bmp,tiff",-1,[](const ProbeOutputContext &ctx) -> std::optional<OutputFormat::OutputPlanT> {

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

        return std::make_tuple(std::unique_ptr<StreamOutputBase>(std::move(strm)), ConversionChain());
#endif
      }
      return std::nullopt;
    }));

    // Add some type conversion functions.
    // Not 1 as we lose range.

    [[maybe_unused]] bool g_reg = registerConversion([](Array<uint8_t,2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg1 = registerConversion([](Array<uint16_t,2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg2 = registerConversion([](Array<float,2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg3 = registerConversion([](Array<double,2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);

  }
}