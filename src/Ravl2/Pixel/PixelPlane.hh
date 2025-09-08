//
// Created on 08/09/2025.
//

#pragma once

#include <memory>
#include <tuple>
#include <utility>
#include <array>
#include <spdlog/spdlog.h>
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Array.hh"
#include "Ravl2/Types.hh"

namespace Ravl2
{
  //! Helper class for handling scaling between master and plane coordinates with arbitrary dimensions
  template <unsigned Dims, int... Scales>
  struct PlaneScale;

  //! Specialisation for empty scales (base case for recursion)
  template <unsigned Dims>
  struct PlaneScale<Dims>
  {
    //! Array of scale factors
    static constexpr std::array<int, 0> scales{};

    //! Default scale factor for any dimension
    static constexpr int scale(unsigned) noexcept { return 1; }

    //! Map from master coordinates to plane coordinates
    [[nodiscard]] static constexpr Index<Dims> masterToPlane(const Index<Dims>& masterCoord) noexcept
    {
      // If no scales are provided, return copy of master coordinates
      return masterCoord;
    }

    //! Map from plane coordinates to master coordinates
    [[nodiscard]] static constexpr Index<Dims> planeToMaster(const Index<Dims>& planeCoord) noexcept
    {
      // If no scales are provided, return copy of plane coordinates
      return planeCoord;
    }

    //! Calculate plane dimensions based on master dimensions
    [[nodiscard]] static constexpr IndexRange<Dims> calculateRange(const IndexRange<Dims>& masterRange) noexcept
    {
      // If no scales are provided, return copy of master range
      return masterRange;
    }
  };

  //! Specialization for variadic scales
  template <unsigned Dims, int FirstScale, int... RestScales>
  struct PlaneScale<Dims, FirstScale, RestScales...>
  {
    //! Array of scale factors
    static constexpr std::array<int, 1 + sizeof...(RestScales)> scales{FirstScale, RestScales...};

    //! Get scale factor for a specific dimension
    static constexpr int scale(unsigned dim) noexcept
    {
      // Use the provided scale for dimensions with explicit scales, default to 1 for others
      if (dim < scales.size()) {
        return scales[dim];
      }
      return 1; // Default scale for dimensions beyond those specified
    }

    //! Map from master coordinates to plane coordinates
    [[nodiscard]] static constexpr Index<Dims> masterToPlane(const Index<Dims>& masterCoord) noexcept
    {
      Index<Dims> planeCoord;
      for (unsigned i = 0; i < Dims; ++i) {
        planeCoord[i] = masterCoord[i] / scale(i);
      }
      return planeCoord;
    }

    //! Map from plane coordinates to master coordinates (top-left corner)
    [[nodiscard]] static constexpr Index<Dims> planeToMaster(const Index<Dims>& planeCoord) noexcept
    {
      Index<Dims> masterCoord;
      for (unsigned i = 0; i < Dims; ++i) {
        masterCoord[i] = planeCoord[i] * scale(i);
      }
      return masterCoord;
    }

    //! Calculate plane dimensions based on master dimensions
    [[nodiscard]] static constexpr IndexRange<Dims> calculateRange(const IndexRange<Dims>& masterRange) noexcept
    {
      IndexRange<Dims> planeRange;
      for (unsigned i = 0; i < Dims; ++i) {
        const int s = scale(i);
        planeRange.min(i) = (masterRange.min(i) + s - 1) / s;
        planeRange.max(i) = (masterRange.max(i) + s - 1) / s;
      }
      return planeRange;
    }
  };

  //! A single plane of pixel data with optional subsampling
  //! @tparam DataT The pixel data type for this plane
  //! @tparam Dims The dimensions of the array
  //! @tparam Channel The channel type this plane represents
  //! @tparam Scales Scaling factors for each dimension (can be less than Dims)
  template <typename DataT, unsigned Dims, ImageChannel Channel, int... Scales>
  class PixelPlane
  {
  public:
    using value_type = DataT;
    using array_type = Array<DataT, Dims>;
    using scale_type = PlaneScale<Dims, Scales...>;

    //! Default constructor
    PixelPlane() = default;

    //! Construct with a specified range
    explicit PixelPlane(const IndexRange<Dims>& range)
      : m_data(range)
    {}

    //! Construct with a specified master range (will be scaled)
    PixelPlane(const IndexRange<Dims>& masterRange, bool scale)
      : m_data(scale ? scale_type::calculateRange(masterRange) : masterRange)
    {}

    //! Construct with a specified range and initial value
    PixelPlane(const IndexRange<Dims>& range, const DataT& initialValue)
      : m_data(range, initialValue)
    {}

    //! Construct with a master range that will be scaled and initial value
    PixelPlane(const IndexRange<Dims>& masterRange, bool scale, const DataT& initialValue)
      : m_data(scale ? scale_type::calculateRange(masterRange) : masterRange, initialValue)
    {}

    //! Get the range of this plane
    [[nodiscard]] const IndexRange<Dims>& range() const noexcept
    {
      return m_data.range();
    }

    //! Access the underlying array
    [[nodiscard]] array_type& data() noexcept
    {
      return m_data;
    }

    //! Access the underlying array (const)
    [[nodiscard]] const array_type& data() const noexcept
    {
      return m_data;
    }

    //! Access an element by index (direct plane coordinates)
    [[nodiscard]] DataT& operator[](const Index<Dims>& index)
    {
      return m_data[index];
    }

    //! Access an element by index (direct plane coordinates, const)
    [[nodiscard]] const DataT& operator[](const Index<Dims>& index) const
    {
      return m_data[index];
    }

    //! Access an element using master coordinates (will be scaled)
    [[nodiscard]] DataT& atMaster(const Index<Dims>& masterIndex)
    {
      auto planeIndex = scale_type::masterToPlane(masterIndex);
      return m_data[planeIndex];
    }

    //! Access an element using master coordinates (will be scaled, const)
    [[nodiscard]] const DataT& atMaster(const Index<Dims>& masterIndex) const
    {
      auto planeIndex = scale_type::masterToPlane(masterIndex);
      return m_data[planeIndex];
    }

    //! Get the master-space range this plane covers
    [[nodiscard]] IndexRange<Dims> masterRange() const
    {
      const auto& planeRange = m_data.range();
      IndexRange<Dims> result;

      for (unsigned i = 0; i < Dims; ++i) {
        const int s = scale_type::scale(i);
        result.min(i) = planeRange.min(i) * s;
        result.max(i) = (planeRange.max(i) + 1) * s - 1;
      }

      return result;
    }

    //! Begin iterator for the plane data
    [[nodiscard]] auto begin() const
    {
      return m_data.begin();
    }

    //! End iterator for the plane data
    [[nodiscard]] auto end() const
    {
      return m_data.end();
    }

    //! Check if the plane contains a master coordinate
    [[nodiscard]] bool containsMaster(const Index<Dims>& masterIndex) const
    {
      auto planeIndex = scale_type::masterToPlane(masterIndex);
      return m_data.range().contains(planeIndex);
    }

    //! Fill the plane with a value
    void fill(const DataT& value)
    {
      for (auto& pixel : m_data) {
        pixel = value;
      }
    }

    //! Get scale factor for a specific dimension
    [[nodiscard]] static constexpr int getScale(unsigned dim) noexcept
    {
      return scale_type::scale(dim);
    }

    //! Get the channel type of this plane
    [[nodiscard]] static constexpr ImageChannel getChannelType() noexcept
    {
      return Channel;
    }

  private:
    array_type m_data;
  };

  //! Collection of image planes with potentially different types and scaling factors
  //! @tparam Dims The number of dimensions for the planar image
  //! @tparam PlaneTypes The types of each plane, including channel and scaling information
  template <unsigned Dims, typename... PlaneTypes>
  class PlanarImage
  {
  public:
    //! Default constructor
    PlanarImage() = default;

    //! Construct with a master range that will be applied to all planes (with appropriate scaling)
    explicit PlanarImage(const IndexRange<Dims>& masterRange)
      : m_planes(std::make_tuple(PlaneTypes(masterRange, true)...))
    {}

    //! Get the number of planes
    [[nodiscard]] static constexpr std::size_t planeCount() noexcept
    {
      return sizeof...(PlaneTypes);
    }

    //! Get a specific plane by index
    template <std::size_t Index>
    [[nodiscard]] auto& plane()
    {
      return std::get<Index>(m_planes);
    }

    //! Get a specific plane by index (const)
    template <std::size_t Index>
    [[nodiscard]] const auto& plane() const
    {
      return std::get<Index>(m_planes);
    }

    //! Get a plane by channel type (compile-time)
    template <ImageChannel Channel>
    [[nodiscard]] decltype(auto) planeByChannel()
    {
        return planeByChannelImpl<Channel>(std::make_index_sequence<sizeof...(PlaneTypes)>{});
    }

    //! Get a plane by channel type (compile-time, const version)
    template <ImageChannel Channel>
    [[nodiscard]] decltype(auto) planeByChannel() const
    {
        return planeByChannelImpl<Channel>(std::make_index_sequence<sizeof...(PlaneTypes)>{});
    }

    //! Get the channel type of a specific plane
    template <std::size_t Index>
    [[nodiscard]] static constexpr ImageChannel planeChannelType() noexcept
    {
      return std::tuple_element_t<Index, std::tuple<PlaneTypes...>>::getChannelType();
    }

    //! Get master dimensions that would cover all planes (taking scaling into account)
    [[nodiscard]] IndexRange<Dims> masterRange() const
    {
      IndexRange<Dims> result = IndexRange<Dims>::mostEmpty();
      using Indices = std::index_sequence_for<PlaneTypes...>;
      applyToEachPlane([&result](const auto& plane) {
        result.involve(plane.masterRange());
      }, Indices{});
      return result;
    }

    //! Apply a function to each plane
    template <typename FuncT>
    void forEachPlane(FuncT&& func)
    {
      using Indices = std::index_sequence_for<PlaneTypes...>;
      applyToEachPlane(std::forward<FuncT>(func), Indices{});
    }

    //! Apply a function to each plane (const)
    template <typename FuncT>
    void forEachPlane(FuncT&& func) const
    {
      using Indices = std::index_sequence_for<PlaneTypes...>;
      applyToEachPlane(std::forward<FuncT>(func), Indices{});
    }

  private:
    // Helper to apply a function to each plane using index sequence
    template <typename FuncT, std::size_t... Is>
    void applyToEachPlane(FuncT&& func, std::index_sequence<Is...>) const
    {
      (func(std::get<Is>(m_planes)), ...);
    }

    // Non-const version
    template <typename FuncT, std::size_t... Is>
    void applyToEachPlane(FuncT&& func, std::index_sequence<Is...>)
    {
      (func(std::get<Is>(m_planes)), ...);
    }

    // Helper to get a plane by channel type
    template <ImageChannel Channel, std::size_t... Is>
    [[nodiscard]] decltype(auto) planeByChannelImpl(std::index_sequence<Is...>)
    {
        // Use fold expression with comma operator to select the correct plane
        // The comma operator evaluates all expressions and returns the last one
        return (... , (planeChannelType<Is>() == Channel ? 
                      std::ref(plane<Is>()) : 
                      (Is == sizeof...(PlaneTypes) - 1 ? 
                          throw std::runtime_error("Channel not found in planar image") : 
                          std::ref(plane<0>()))));
    }

    // Const version
    template <ImageChannel Channel, std::size_t... Is>
    [[nodiscard]] decltype(auto) planeByChannelImpl(std::index_sequence<Is...>) const
    {
        return (... , (planeChannelType<Is>() == Channel ? 
                      std::ref(plane<Is>()) : 
                      (Is == sizeof...(PlaneTypes) - 1 ? 
                          throw std::runtime_error("Channel not found in planar image") : 
                          std::ref(plane<0>()))));
    }

    //! Check if a specific plane contains a master coordinate
    template <std::size_t PlaneIndex>
    [[nodiscard]] bool containsMaster(const Index<Dims>& masterIndex) const
    {
      return std::get<PlaneIndex>(m_planes).containsMaster(masterIndex);
    }

    //! Helper for getting a component value from the appropriate plane
    template <ImageChannel Channel, std::size_t PlaneIndex>
    [[nodiscard]] auto getChannelFromPlane(const Index<Dims>& masterIndex) const
    {
      return std::get<PlaneIndex>(m_planes).atMaster(masterIndex);
    }

    std::tuple<PlaneTypes...> m_planes;

  };

  namespace detail
  {
    // Implementation helper for convertToPlanar that unpacks the channels from PixelT
    template <unsigned Dims, typename PixelT, std::size_t... Is>
    static auto convertToPlanarImpl(const Array<PixelT, Dims>& packedArray, std::index_sequence<Is...>)
    {
      // Get the component type from the pixel
      using ComponentT = typename PixelT::value_type;

      // Create a planar image with appropriate plane types for each channel
      using PlanarType = PlanarImage<Dims, PixelPlane<ComponentT, Dims, PixelT::template getChannelAtIndex<Is>()>...>;

      // Create the planar image with the same range as the packed array
      PlanarType result(packedArray.range());

      // Extract data for each pixel
      for (auto it = packedArray.begin(); it != packedArray.end(); ++it) {
        const auto& pixel = *it;
        const auto& idx = it.index();

        // Simply extract values by index position and assign to corresponding plane
        // Since both pixel and planes have the same component type, this is safe
        ((result.template plane<Is>()[idx] = pixel[Is]), ...);
      }

      return result;
    }
  }

  //! Convert a packed pixel array to a planar image
  //! This generic implementation extracts channels from the pixel type and creates a corresponding planar image
  //! @tparam Dims The number of dimensions
  //! @tparam PixelT The packed pixel type (e.g., PixelRGB8)
  template <unsigned Dims, typename PixelT>
  auto convertToPlanar(const Array<PixelT, Dims>& packedArray)
  {
    // Use the channel_count from the Pixel class
    return detail::convertToPlanarImpl<Dims>(packedArray, std::make_index_sequence<PixelT::channel_count>{});
  }


  //! Helper alias for 2D planar images (most common case)
  template <typename... PlaneTypes>
  using PlanarImage2D = PlanarImage<2, PlaneTypes...>;

  //! Helper type for creating YUV 4:4:4 planar images (no subsampling)
  template <typename ComponentT>
  using YUV444Image = PlanarImage2D<
    PixelPlane<ComponentT, 2, ImageChannel::Luminance, 1, 1>, // Y plane (full resolution)
    PixelPlane<ComponentT, 2, ImageChannel::ChrominanceU, 1, 1>, // U plane (full resolution)
    PixelPlane<ComponentT, 2, ImageChannel::ChrominanceV, 1, 1>  // V plane (full resolution)
  >;

  //! Helper type for creating YUV 4:2:2 planar images (horizontal subsampling)
  template <typename ComponentT>
  using YUV422Image = PlanarImage2D<
    PixelPlane<ComponentT, 2, ImageChannel::Luminance, 1, 1>, // Y plane (full resolution)
    PixelPlane<ComponentT, 2, ImageChannel::ChrominanceU, 2, 1>, // U plane (half horizontal resolution)
    PixelPlane<ComponentT, 2, ImageChannel::ChrominanceV, 2, 1>  // V plane (half horizontal resolution)
  >;

  //! Helper type for creating YUV 4:2:0 planar images (horizontal and vertical subsampling)
  template <typename ComponentT>
  using YUV420Image = PlanarImage2D<
    PixelPlane<ComponentT, 2, ImageChannel::Luminance, 1, 1>, // Y plane (full resolution)
    PixelPlane<ComponentT, 2, ImageChannel::ChrominanceU, 2, 2>, // U plane (half resolution in both dimensions)
    PixelPlane<ComponentT, 2, ImageChannel::ChrominanceV, 2, 2>  // V plane (half resolution in both dimensions)
  >;

  //! Helper type for creating RGB planar images
  template <typename ComponentT>
  using RGBPlanarImage = PlanarImage2D<
    PixelPlane<ComponentT, 2, ImageChannel::Red>, // R plane
    PixelPlane<ComponentT, 2, ImageChannel::Green>, // G plane
    PixelPlane<ComponentT, 2, ImageChannel::Blue>  // B plane
  >;

  //! Helper type for creating RGBA planar images
  template <typename ComponentT>
  using RGBAPlanarImage = PlanarImage2D<
    PixelPlane<ComponentT, 2, ImageChannel::Red>, // R plane
    PixelPlane<ComponentT, 2, ImageChannel::Green>, // G plane
    PixelPlane<ComponentT, 2, ImageChannel::Blue>, // B plane
    PixelPlane<ComponentT, 2, ImageChannel::Alpha>  // A plane
  >;

  //! Example for a 3D volume with planar color channels
  template <typename ComponentT>
  using RGBVolumeImage = PlanarImage<3,
    PixelPlane<ComponentT, 3, ImageChannel::Red>, // R volume
    PixelPlane<ComponentT, 3, ImageChannel::Green>, // G volume
    PixelPlane<ComponentT, 3, ImageChannel::Blue>  // B volume
  >;

}// namespace Ravl2
