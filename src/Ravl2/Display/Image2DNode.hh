#pragma once

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <vector>

#include "Ravl2/Display/Image2DNodeBase.hh"
#include "Ravl2/Display/Normalization.hh"
#include "Ravl2/Pixel/Pixel.hh"

namespace Ravl2::DebugDisplay {

//! Templated 2D image node for type-specific pixel storage.
//! Specializations provide type-specific formatting and GPU upload.
template<typename T>
class Image2DNode : public Image2DNodeBase {
protected:
  std::vector<T> pixelData;  //!< Original typed data preserved

  //! Type-specific value formatting (can be overridden in specializations)
  virtual std::string formatValue(const T& value) const {
    if constexpr (std::is_integral_v<T>) {
      return std::format("Value: {}", value);
    } else if constexpr (std::is_floating_point_v<T>) {
      return std::format("Value: {:.3f}", value);
    } else {
      return "Value: <unknown>";
    }
  }

  //! Optional extra information (can be overridden in specializations)
  virtual std::string formatExtra([[maybe_unused]] const T& value) const {
    return "";
  }

public:
  Image2DNode() = default;
  ~Image2DNode() override = default;

  //! Set image data
  void setData(std::vector<T> data, int w, int h) {
    pixelData = std::move(data);
    width = w;
    height = h;
#if defined(RAVL2_WITH_BGFX)
    gpuDirty = true;
#endif
  }

  //! Set image data from raw pointer
  void setData(const T* data, int w, int h) {
    const size_t sz = static_cast<size_t>(w) * static_cast<size_t>(h);
    pixelData.assign(data, data + sz);
    width = w;
    height = h;
#if defined(RAVL2_WITH_BGFX)
    gpuDirty = true;
#endif
  }

  //! Query pixel information at (x, y)
  PixelQueryResult queryPixelInfo(int x, int y) const override {
    if (x < 0 || x >= width || y < 0 || y >= height) {
      return {.valid = false, .coordinateText = "", .valueText = "", .extraInfo = std::nullopt};
    }

    const T& value = pixelData[static_cast<size_t>(y * width + x)];
    std::string extra = formatExtra(value);

    return {
      .valid = true,
      .coordinateText = std::format("x: {}, y: {}", x, y),
      .valueText = formatValue(value),
      .extraInfo = extra.empty() ? std::nullopt : std::optional(extra)
    };
  }

  //! Direct access for advanced use cases
  const std::vector<T>& getData() const { return pixelData; }
  const T& samplePixel(int x, int y) const {
    return pixelData[static_cast<size_t>(y * width + x)];
  }
};

//! Specialization for uint8_t (grayscale images)
template<>
class Image2DNode<uint8_t> : public Image2DNodeBase {
private:
  std::vector<uint8_t> pixelData;

protected:
  std::string formatValue(const uint8_t& value) const {
    return std::format("Value: {}", value);
  }

public:
  Image2DNode() = default;
  ~Image2DNode() override = default;

  void setData(std::vector<uint8_t> data, int w, int h);
  void setData(const uint8_t* data, int w, int h);

  // Legacy interface for backward compatibility
  void setFromU8(const uint8_t* src, int w, int h) { setData(src, w, h); }

  PixelQueryResult queryPixelInfo(int x, int y) const override;
  void uploadToGPU() override;

  const std::vector<uint8_t>& getData() const { return pixelData; }
  const uint8_t& samplePixel(int x, int y) const {
    return pixelData[static_cast<size_t>(y * width + x)];
  }

  // Sample original value (for backward compatibility)
  std::pair<float, float> sample(int x, int y) const noexcept;
};

//! Specialization for float (grayscale float images with normalization)
template<>
class Image2DNode<float> : public Image2DNodeBase {
private:
  std::vector<float> pixelData;
  NormalizationSettings norm{};
  float cachedMin = 0.0f;
  float cachedMax = 1.0f;

protected:
  std::string formatValue(const float& value) const;
  std::string formatExtra(const float& value) const;

public:
  Image2DNode() = default;
  ~Image2DNode() override = default;

  void setData(std::vector<float> data, int w, int h);
  void setData(const float* data, int w, int h);

  // Legacy interface for backward compatibility
  void setFromF32(const float* src, int w, int h) { setData(src, w, h); }

  void setNormalization(const NormalizationSettings& settings) {
    norm = settings;
#if defined(RAVL2_WITH_BGFX)
    gpuDirty = true;
#endif
  }

  const NormalizationSettings& getNormalization() const { return norm; }

  PixelQueryResult queryPixelInfo(int x, int y) const override;
  void uploadToGPU() override;

  const std::vector<float>& getData() const { return pixelData; }
  const float& samplePixel(int x, int y) const {
    return pixelData[static_cast<size_t>(y * width + x)];
  }

  // Sample original value (for backward compatibility)
  std::pair<float, float> sample(int x, int y) const noexcept;

  // For compatibility with existing code
  float getCachedMin() const { return cachedMin; }
  float getCachedMax() const { return cachedMax; }
};

//! Specialization for PixelRGB8 (RGB color images)
template<>
class Image2DNode<PixelRGB8> : public Image2DNodeBase {
private:
  std::vector<PixelRGB8> pixelData;

protected:
  std::string formatValue(const PixelRGB8& value) const;

public:
  Image2DNode() = default;
  ~Image2DNode() override = default;

  void setData(std::vector<PixelRGB8> data, int w, int h);
  void setData(const PixelRGB8* data, int w, int h);

  PixelQueryResult queryPixelInfo(int x, int y) const override;
  void uploadToGPU() override;

  const std::vector<PixelRGB8>& getData() const { return pixelData; }
  const PixelRGB8& samplePixel(int x, int y) const {
    return pixelData[static_cast<size_t>(y * width + x)];
  }
};

//! Specialization for int16_t (label/ID images with smaller range)
template<>
class Image2DNode<int16_t> : public Image2DNodeBase {
private:
  std::vector<int16_t> pixelData;

protected:
  std::string formatValue(const int16_t& value) const;

public:
  Image2DNode() = default;
  ~Image2DNode() override = default;

  void setData(std::vector<int16_t> data, int w, int h);
  void setData(const int16_t* data, int w, int h);

  PixelQueryResult queryPixelInfo(int x, int y) const override;
  void uploadToGPU() override;

  const std::vector<int16_t>& getData() const { return pixelData; }
  const int16_t& samplePixel(int x, int y) const {
    return pixelData[static_cast<size_t>(y * width + x)];
  }
};

//! Specialization for int32_t (label/ID images with full range)
template<>
class Image2DNode<int32_t> : public Image2DNodeBase {
private:
  std::vector<int32_t> pixelData;

protected:
  std::string formatValue(const int32_t& value) const;

public:
  Image2DNode() = default;
  ~Image2DNode() override = default;

  void setData(std::vector<int32_t> data, int w, int h);
  void setData(const int32_t* data, int w, int h);

  PixelQueryResult queryPixelInfo(int x, int y) const override;
  void uploadToGPU() override;

  const std::vector<int32_t>& getData() const { return pixelData; }
  const int32_t& samplePixel(int x, int y) const {
    return pixelData[static_cast<size_t>(y * width + x)];
  }
};

} // namespace Ravl2::DebugDisplay
