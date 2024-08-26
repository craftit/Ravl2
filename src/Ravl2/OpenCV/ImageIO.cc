//
// Created by charles galambos on 25/08/2024.
//

#include <opencv4/opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

#include "Ravl2/OpenCV/Image.hh"
#include "Ravl2/OpenCV/ImageIO.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/InputStreamContainer.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Pixel/Colour.hh"
#include "Ravl2/OpenCV/Display.hh"

namespace Ravl2
{
  void initOpenCVImageIO()
  {
    initOpenCVDisplay();
    initColourConversion();
  }
  namespace
  {

    //! Loading images.

    template <typename ViaPixelT, unsigned N = 2>
    std::optional<ConversionChain> conversionPlan(const ProbeInputContext &ctx)
    {
      using ViaT = Ravl2::Array<ViaPixelT, N>;
      std::optional<ConversionChain> convChain = typeConverterMap().find(ctx.m_targetType, typeid(ViaT));
      if(!convChain.has_value()) {
        SPDLOG_WARN("No conversion chain found for {} <- {}",typeName(ctx.m_targetType), typeName(typeid(ViaT)));
        return std::nullopt;
      }
      static auto convType = makeTypeConversion<ViaT,cv::Mat>([](cv::Mat img) -> ViaT { return toArray<ViaPixelT,2>(img); },1.0f);
      return convChain.value().prepend(convType);
    }


    //! Loading images.

    template <typename ViaPixelT, unsigned N = 2>
    std::optional<StreamInputPlan> makePlan(cv::Mat img, const ProbeInputContext &ctx)
    {
      using ViaT = Ravl2::Array<ViaPixelT, N>;
      std::optional<ConversionChain> convChain = typeConverterMap().find(ctx.m_targetType, typeid(ViaT));
      if(!convChain.has_value())
        return std::nullopt;
      return StreamInputPlan {std::make_shared<InputStreamContainer<ViaT>>(toArray<ViaPixelT, N>(img)), convChain.value(), convChain.value().conversionLoss()};
    }

    //! Loading images.

    [[maybe_unused]] bool g_regFmt = inputFormatMap().add(std::make_shared<InputFormatCall>("OpenCV", "png,jpg,jpeg,bmp,tiff", "file", -1, [](const ProbeInputContext &ctx) -> std::optional<StreamInputPlan> {
      //! If we are looking for a cv::Mat, we can just read the file directly.
      if(ctx.m_targetType == typeid(cv::Mat)) {
        auto strm = std::make_shared<StreamInputCall<cv::Mat>>([filename = ctx.m_filename](std::streampos &pos) -> std::optional<cv::Mat> {
          if(pos != 0)
            return std::nullopt;
          return cv::imread(filename, cv::IMREAD_UNCHANGED);
        });
        if(ctx.m_verbose) {
          SPDLOG_INFO("Plan made for opened OpenCV image file: {}", ctx.m_filename);
        }
        return StreamInputPlan {strm, {}, 1.0f};
      }
      // Apply some heuristics to determine the type of the image we want to load.
      // Ideally this would be delt with in the type converter, but opencv gives
      // us no information about the colour space of the image.
      cv::ImreadModes readMode = cv::IMREAD_UNCHANGED;
      if(ctx.m_targetType == typeid(Array<uint8_t, 2>) || ctx.m_targetType == typeid(Array<int8_t, 2>)
         || ctx.m_targetType == typeid(Array<uint16_t, 2>) || ctx.m_targetType == typeid(Array<int16_t, 2>)
         || ctx.m_targetType == typeid(Array<int32_t, 2>) || ctx.m_targetType == typeid(Array<float, 2>)
         || ctx.m_targetType == typeid(Array<double, 2>)) {
        readMode = cv::IMREAD_GRAYSCALE;
      }

      // The best we can do is loaded it directly and look for a conversion.
      cv::Mat img = cv::imread(ctx.m_filename, readMode);
      if(img.empty()) {
        if(ctx.m_verbose) {
          SPDLOG_INFO("Failed to load image: {}", ctx.m_filename);
        }
        return std::nullopt;
      }

      switch(img.type()) {
        case CV_8UC1: {
          return makePlan<uint8_t>(img, ctx);
        }
        case CV_8SC1: {
          return makePlan<int8_t>(img, ctx);
        }
        case CV_16UC1: {
          return makePlan<uint16_t>(img, ctx);
        }
        case CV_16SC1: {
          return makePlan<int16_t>(img, ctx);
        }
        case CV_32SC1: {
          return makePlan<int32_t>(img, ctx);
        }
        case CV_32FC1: {
          return makePlan<float>(img, ctx);
        }
        case CV_64FC1: {
          return makePlan<double>(img, ctx);
        }
        default: break;
      }

      int depth = CV_MAT_DEPTH(img.type());
      int channels = CV_MAT_CN(img.type());
      int baseType = CV_MAT_TYPE(img.type());

      SPDLOG_WARN("Don't know how to convert OpenCV image type: {}.  Depth:{} Channels:{} baseType:{}", img.type(), depth, channels, baseType);
      return std::nullopt;
    }));

    //! Saving images.

    [[maybe_unused]] bool g_regFmt1 = outputFormatMap().add(std::make_shared<OutputFormatCall>("OpenCV", "png,jpg,jpeg,bmp,tiff", "file", -1, [](const ProbeOutputContext &ctx) -> std::optional<StreamOutputPlan> {
      //! If we are looking for a cv::Mat, we can just read the file directly.
      if(ctx.m_sourceType == typeid(cv::Mat)) {
        auto strm = std::make_unique<StreamOutputCall<cv::Mat>>([filename = ctx.m_filename](const cv::Mat &img, std::streampos pos) -> std::streampos {
          if(pos != 0) {
            throw std::runtime_error("OpenCV output format does not support seeking.");
          }
          cv::imwrite(filename, img);
          return 0;
        });
        return StreamOutputPlan {std::move(strm), {}, 1.0f};
      }
      return std::nullopt;
    }));


    std::optional<StreamInputPlan> makeVideoCapturePlan(cv::VideoCapture &videoCapture,const ProbeInputContext &ctx)
    {
      // We need to read a frame to get the type of the video.
      cv::Mat firstFrame;
      videoCapture >> firstFrame;

      std::optional<ConversionChain> thePlan;
      switch(firstFrame.type()) {
        case CV_8UC1: {
          thePlan = conversionPlan<uint8_t>(ctx);
        } break;
        case CV_8SC1: {
          thePlan = conversionPlan<int8_t>(ctx);
        } break;
        case CV_16UC1: {
          thePlan = conversionPlan<uint16_t>(ctx);
        } break;
        case CV_16SC1: {
          thePlan = conversionPlan<int16_t>(ctx);
        } break;
        case CV_32SC1: {
          thePlan = conversionPlan<int32_t>(ctx);
        } break;
        case CV_32FC1: {
          thePlan = conversionPlan<float>(ctx);
        } break;
        case CV_64FC1: {
          thePlan = conversionPlan<double>(ctx);
        } break;
        case CV_8UC3: {
          thePlan = conversionPlan<PixelBGR8>(ctx);
        } break;
        default: break;
      }

      if(!thePlan.has_value()) {
        int depth = CV_MAT_DEPTH(firstFrame.type());
        int channels = CV_MAT_CN(firstFrame.type());
        int baseType = CV_MAT_TYPE(firstFrame.type());

        SPDLOG_WARN("Don't know how to convert OpenCV image type: {}.  Depth:{} Channels:{} baseType:{} ({})", firstFrame.type(), depth, channels, baseType,CV_8UC3);
        return std::nullopt;
      }

      auto strm = std::make_shared<StreamInputCall<cv::Mat>>([videoCapture,firstFrame,isFirst=true](std::streampos &pos) mutable -> std::optional<cv::Mat> {
        if(pos != 0) {
          return std::nullopt;
        }
        if(isFirst) {
          isFirst = false;
          return firstFrame;
        }
        videoCapture >> firstFrame;
        if(firstFrame.empty()) {
          return std::nullopt;
        }
        return firstFrame;
      });
      return StreamInputPlan {strm, thePlan.value(), thePlan.value().conversionLoss()};
    }

    //! Load a video file
    [[maybe_unused]] bool g_regFmt2 = inputFormatMap().add(std::make_shared<InputFormatCall>("OpenCV", "avi,mp4,mov", "file", -1, [](const ProbeInputContext &ctx) -> std::optional<StreamInputPlan> {
      cv::VideoCapture videoCapture ;
      if(!videoCapture.open(ctx.m_filename)) {
        if(ctx.m_verbose) {
          SPDLOG_INFO("Failed to open video stream '{}'", ctx.m_filename);
        }
        return std::nullopt;
      }
      return makeVideoCapturePlan(videoCapture, ctx);
    }));

    //! Open a webcam
    [[maybe_unused]] bool g_regFmt3 = inputFormatMap().add(std::make_shared<InputFormatCall>("OpenCV", "", "camera", -1, [](const ProbeInputContext &ctx) -> std::optional<StreamInputPlan> {
      cv::VideoCapture videoCapture ;
      // Convert the filename to an integer.
      int cameraId = std::stoi(ctx.m_filename);
      if(!videoCapture.open(cameraId)) {
        if(ctx.m_verbose) {
          SPDLOG_INFO("Failed to open video stream '{}'", ctx.m_filename);
        }
        return std::nullopt;
      }
      return makeVideoCapturePlan(videoCapture, ctx);
    }));

    // Add some type conversion functions.
    // They don't have a loss of  1 as we lose range information.

    [[maybe_unused]] bool g_reg = registerConversion([](Array<uint8_t, 2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg1 = registerConversion([](Array<int8_t, 2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg2 = registerConversion([](Array<uint16_t, 2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg3 = registerConversion([](Array<int16_t, 2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg4 = registerConversion([](Array<int32_t, 2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg5 = registerConversion([](Array<float, 2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);
    [[maybe_unused]] bool g_reg6 = registerConversion([](Array<double, 2> img) -> cv::Mat { return toCvMat(img); }, 0.95f);

  }// namespace
}// namespace Ravl2