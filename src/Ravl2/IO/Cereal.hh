//
// Created by charles galambos on 24/08/2024.
//

#pragma once

#ifndef CEREAL_THREAD_SAFE
#message "CEREAL_THREAD_SAFE not defined. Defining it."
#define CEREAL_THREAD_SAFE 1
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
// The compiler claims this has some potential null dereference issues.
#include <cereal/archives/binary.hpp>
#pragma GCC diagnostic pop
#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <utility>
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/IO/Load.hh"

namespace Ravl2
{
  namespace detail
  {
    //! Helper to serialize the content of an array without the range.

    template <typename ArrayT>
    struct CerealMatrixBlock {
      CerealMatrixBlock() = default;

      explicit CerealMatrixBlock(ArrayT &data)
          : mData(&data)
      {}

      template <typename ArchiveT>
      void save(ArchiveT & archive) const
      {
        assert(mData != nullptr);
        auto rows = mData->rows();
        auto cols = mData->cols();
        if constexpr(std::is_same_v<ArchiveT, cereal::JSONOutputArchive>) {
          cereal::size_type buffSize = cereal::size_type(mData->rows() * mData->cols());
          cereal::size_type size = buffSize;
          archive(cereal::make_size_tag(size));
          if(size != buffSize) {
            throw std::out_of_range("unexpected size");
          }
        }
        for(int i = 0;i < rows;i++) {
          for(int j = 0; j < cols; j++) {
            archive((*mData)(i, j));
          }
        }
      }

      template <typename ArchiveT>
      void load(ArchiveT & archive)
      {
        assert(mData != nullptr);
        auto rows = mData->rows();
        auto cols = mData->cols();
        if constexpr(std::is_same_v<ArchiveT, cereal::JSONInputArchive>) {
          cereal::size_type buffSize = cereal::size_type(rows * cols);
          cereal::size_type size = buffSize;
          archive(cereal::make_size_tag(size));
          if(size != buffSize) {
            SPDLOG_ERROR("Size mismatch: {} != {}", size, buffSize);
            throw std::out_of_range("unexpected size");
          }
        }
        for(int i = 0;i < rows;i++) {
          for(int j = 0; j < cols; j++) {
            archive((*mData)(i, j));
          }
        }
      }

      ArrayT *mData = nullptr;
    };

  }// namespace detail
}


namespace cereal
{
#if 0
  template <class Archive, class Derived> inline
  typename std::enable_if<traits::is_output_serializable<BinaryData<typename Derived::Scalar>, Archive>::value, void>::type
  save(Archive & ar, Eigen::PlainObjectBase<Derived> const & m){
    typedef Eigen::PlainObjectBase<Derived> ArrT;
    if(ArrT::RowsAtCompileTime==Eigen::Dynamic) ar(m.rows());
    if(ArrT::ColsAtCompileTime==Eigen::Dynamic) ar(m.cols());
    ar(binary_data(m.data(),size_t(m.size())*sizeof(typename Derived::Scalar)));
  }

  template <class Archive, class Derived> inline
  typename std::enable_if<traits::is_input_serializable<BinaryData<typename Derived::Scalar>, Archive>::value, void>::type
  load(Archive & ar, Eigen::PlainObjectBase<Derived> & m){
    typedef Eigen::PlainObjectBase<Derived> ArrT;
    Eigen::Index rows=ArrT::RowsAtCompileTime, cols=ArrT::ColsAtCompileTime;
    if(rows==Eigen::Dynamic) ar(rows);
    if(cols==Eigen::Dynamic) ar(cols);
    m.resize(rows,cols);
    ar(binary_data(m.data(),size_t(rows)*size_t(cols)*sizeof(typename Derived::Scalar)));
  }
#endif

#if 1
  template <class Archive, class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols> inline
  void save(Archive & ar, Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols> const & m)
  {
    int32_t rows = int32_t(m.rows());
    int32_t cols = int32_t(m.cols());
    if(_Rows == Eigen::Dynamic) {
      ar(cereal::make_nvp("rows",rows));
    }
    if(_Cols == Eigen::Dynamic) {
      ar(cereal::make_nvp("cols",cols));
    }
    // Check if this is a json archive.
    if constexpr (traits::is_output_serializable<BinaryData<_Scalar>, Archive>::value && !std::is_same_v<Archive, cereal::JSONOutputArchive>) {
      ar(binary_data(m.data(),size_t(rows)*size_t(cols)*sizeof(_Scalar)));
    } else {
      Ravl2::detail::CerealMatrixBlock<const Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>> blk(m);
      ar(cereal::make_nvp("data", blk));
    }
  }

  template <class Archive, class _Scalar, int _Rows, int _Cols, int _Options, int _MaxRows, int _MaxCols> inline
  void load(Archive & ar, Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols> & m)
  {
    int32_t rows = _Rows;
    int32_t cols = _Cols;
    if constexpr(_Rows == Eigen::Dynamic) {
      ar(cereal::make_nvp("rows",rows));
      if constexpr(_Cols == Eigen::Dynamic) {
        ar(cereal::make_nvp("cols",cols));
      }
      m.resize(rows,cols);
    } else {
      if constexpr(_Cols == Eigen::Dynamic) {
        ar(cereal::make_nvp("cols",cols));
        m.resize(rows,cols);
      }
    }
    if constexpr (traits::is_input_serializable<BinaryData<_Scalar>, Archive>::value && !std::is_same_v<Archive, cereal::JSONInputArchive>) {
      ar(binary_data(m.data(),size_t(rows)*size_t(cols)*sizeof(_Scalar)));
    } else {
      Ravl2::detail::CerealMatrixBlock<Eigen::Matrix<_Scalar, _Rows, _Cols, _Options, _MaxRows, _MaxCols>> blk(m);
      ar(cereal::make_nvp("data", blk));
    }
  }
#endif

}

namespace Ravl2
{
  //! Enable cerial IO
  bool initCerealIO();

  //! @brief Header for a cereal archive.
  struct CerealArchiveHeader {
    constexpr static uint32_t m_magicNumber = 0xABBA2024;
    CerealArchiveHeader() = default;
    explicit CerealArchiveHeader(std::string theTypeName);
    uint32_t m_magic = 0;
    uint16_t fileFormatVersion = 0;
    std::string libVersion;
    std::string gitHash;
    std::string typeName;

    template<class Archive>
    void save(Archive & archive) const
    {
      archive(cereal::make_nvp("magic", m_magic),
	      cereal::make_nvp("formatVersion", fileFormatVersion),
	      cereal::make_nvp("version", libVersion),
	      cereal::make_nvp("gitHash", gitHash),
	      cereal::make_nvp("typeName", typeName));
    }

    template<class Archive>
    void load(Archive & archive)
    {
      archive(cereal::make_nvp("magic", m_magic));
      if(m_magic != m_magicNumber) {
	throw std::runtime_error("Magic number mismatch in stream.");
      }
      archive(cereal::make_nvp("formatVersion", fileFormatVersion),
	      cereal::make_nvp("version", libVersion),
	      cereal::make_nvp("gitHash", gitHash),
	      cereal::make_nvp("typeName", typeName));
    }
  };

  //! @brief Archive that uses cereal to write objects to a stream.
  //! @tparam ObjectT - The type of object to write.
  template <typename ObjectT, typename ArchiveT>
  class StreamOutputCerealArchive : public StreamOutput<ObjectT>
  {
  public:
    explicit StreamOutputCerealArchive(std::shared_ptr<std::ostream> stream)
    : m_stream(std::move(stream)),
      m_archive(*m_stream)
    {}

    std::streampos write(const ObjectT &obj, std::streampos pos) override
    {
      // We can't seek in a JSON or XML stream.
      if constexpr(std::is_same_v<ArchiveT, cereal::BinaryOutputArchive>) {
	if(pos != std::numeric_limits<std::streampos>::max()) {
	  m_stream->seekp(pos);
	}
	if(pos == 0) {
	  m_first = true;
	}
      }
      if(m_first) {
	// Write the header.
	m_archive(cereal::make_nvp("header",CerealArchiveHeader(typeName(typeid(ObjectT)))));
	m_first = false;
	if constexpr(std::is_same_v<ArchiveT, cereal::BinaryOutputArchive>) {
	  // Update the start position.
	  this->mStart = m_stream->tellp();
	}
      }
      if(pos == std::numeric_limits<std::streampos>::max()) {
	pos = this->mEnd;
      }
      m_archive(cereal::make_nvp(fmt::format("body{}",size_t(pos)),obj));
      if constexpr(std::is_same_v<ArchiveT, cereal::BinaryInputArchive>) {
	auto at = m_stream->tellp();
	if(at > this->mEnd) {
	  this->mEnd = at;
	}
	return at;
      } else {
	this->mEnd += 1;
	return pos + std::streampos(1);
      }
    }

  private:
    bool m_first = true;
    std::shared_ptr<std::ostream> m_stream; //!< The stream to write to, must be before m_archive to ensure it is destroyed after.
    ArchiveT m_archive;
  };

  //! @brief Archive that uses cereal to write objects to a stream.
  //! @tparam ObjectT - The type of object to write.
  template <typename ObjectT, typename ArchiveT>
  class StreamInputCerealArchive : public StreamInput<ObjectT>
  {
  public:
    explicit StreamInputCerealArchive(std::shared_ptr<std::istream> stream)
        : m_archive(*stream),
          m_stream(std::move(stream))
    {}

    //! Goto next position in the stream and read the object.
    //! @param pos - The position in the stream where the object was written.
    //! @return The object.
    std::optional<ObjectT> next(std::streampos &pos) final
    {
      if(m_stream->eof())
        return std::nullopt;
      if constexpr(std::is_same_v<ArchiveT, cereal::BinaryInputArchive>) {
	if(pos != std::numeric_limits<std::streampos>::max())
	  m_stream->seekg(pos);
	if(pos == 0) {
	  m_isFirst = true;
	}
      }
      if(m_isFirst) {
	// At pos 0, read the header.
	m_archive(cereal::make_nvp("header",m_header));
	m_isFirst = false;
      }
      ObjectT obj;
      m_archive(cereal::make_nvp(fmt::format("body{}", static_cast<std::size_t>(pos)),obj));
      if constexpr(std::is_same_v<ArchiveT, cereal::BinaryInputArchive>) {
	pos = m_stream->tellg();
      } else {
	pos += 1;
      }
      return obj;
    }

  private:
    bool m_isFirst = true;
    ArchiveT m_archive;
    CerealArchiveHeader m_header;
    std::shared_ptr<std::istream> m_stream;
  };

  //! @brief File format for saving objects to a binary file using cereal.
  template <typename ArchiveT>
  class CerealSaveFormat : public OutputFormat
  {
  public:
    explicit CerealSaveFormat(std::string ext = "bin")
        : OutputFormat(fmt::format("Cereal-{}", typeName(typeid(ArchiveT))), ext, "file")
    {}

    template<typename ObjectT>
    bool registerObjectType()
    {
      std::lock_guard lock(m_mutex);
      m_streamOutputFactory[typeid(ObjectT)] = [](const ProbeOutputContext &ctx)
      {
	auto stream = std::make_shared<std::ofstream>(ctx.m_filename, std::ios::binary);
	return std::make_shared<StreamOutputCerealArchive<ObjectT, ArchiveT>>(stream);
      };
      return true;
    }

    //! Provide a format for the archive.s
    static std::shared_ptr<CerealSaveFormat<ArchiveT> > &format() {
      static std::shared_ptr<CerealSaveFormat<ArchiveT> > format = []()
      {
	std::string defaultExt = []() {
	  if constexpr(std::is_same_v<ArchiveT, cereal::BinaryOutputArchive>) {
	    return "xbs";
	  } else if constexpr(std::is_same_v<ArchiveT, cereal::JSONOutputArchive>) {
	    return "json";
	  }
	  return "bin";
	}();
	auto ret = std::make_shared<CerealSaveFormat<ArchiveT>>(defaultExt);
	SPDLOG_TRACE("Registering CerealSaveFormat '{}' with default extension '{}'", typeName(typeid(CerealSaveFormat<ArchiveT>)),defaultExt);
	outputFormatMap().add(ret);
	return ret;
      }();
      return format;
    }

    //! Register a type with the format.
    template<typename ObjectT>
    static bool registerType()
    {
      return format()->template registerObjectType<ObjectT>();
    }

      //! Test if we can save this type.
    [[nodiscard]] std::optional<StreamOutputPlan> probe(const ProbeOutputContext &ctx) final
    {
      // We need a lock, as we are accessing the factory.
      std::shared_lock lock(m_mutex);
      auto it = m_streamOutputFactory.find(ctx.m_sourceType);
      if(it != m_streamOutputFactory.end()) {
	return StreamOutputPlan {it->second(ctx), {}, 1.0f};
      }
      // Make a set of all the types we can convert to.
      std::unordered_set<std::type_index> toTypes;
      for(const auto &pair : m_streamOutputFactory) {
	toTypes.insert(pair.first);
      }
      // See if we can convert to one of the types we know about.
      std::optional<ConversionChain> conv = typeConverterMap().find(ctx.m_sourceType, toTypes);
      if(!conv.has_value()) {
	return std::nullopt;
      }
      it = m_streamOutputFactory.find(conv.value().to());
      if(it == m_streamOutputFactory.end()) {
	SPDLOG_ERROR("Internal error. Conversion chain ends in unknown type.");
	return std::nullopt;
      }
      return StreamOutputPlan {it->second(ctx), conv.value(), conv.value().conversionLoss()};
    }

  private:
    std::shared_mutex m_mutex;
    std::unordered_map<std::type_index,std::function<std::shared_ptr<StreamOutputBase>(const ProbeOutputContext &ctx)> > m_streamOutputFactory;
  };

  //! @brief Cereal load format.
  template <typename ArchiveT>
  class CerealLoadFormat : public InputFormat
  {
  public:
    explicit CerealLoadFormat(std::string ext = "xbs")
      : InputFormat(fmt::format("Cereal-{}", typeName(typeid(ArchiveT))), ext, "file")
    {}

    //! Register a type with the format.
    template<typename ObjectT>
    bool registerObjectType()
    {
      std::lock_guard lock(m_mutex);
      m_streamInputFactory[typeName(typeid(ObjectT))] = [](const ProbeInputContext &ctx) {
	auto stream = std::make_shared<std::ifstream>(ctx.m_filename, std::ios::binary);
	return std::make_unique<StreamInputCerealArchive<ObjectT, ArchiveT>>(std::move(stream));
      };
      return true;
    }

    //! Provide one instance of the format.
    static std::shared_ptr<CerealLoadFormat<ArchiveT> > &format()
    {
      static std::shared_ptr<CerealLoadFormat<ArchiveT> > format = []() {
	std::string defaultExt = []() {
	  if constexpr(std::is_same_v<ArchiveT, cereal::BinaryInputArchive>) {
	    return "xbs";
	  } else if constexpr(std::is_same_v<ArchiveT, cereal::JSONInputArchive>) {
	    return "json";
	  }
	  return "bin";
	}();
	SPDLOG_TRACE("Registering CerealLoadFormat '{}' with default extension '{}'", typeName(typeid(CerealLoadFormat<ArchiveT>)),defaultExt);
	auto ret = std::make_shared<CerealLoadFormat<ArchiveT>>(defaultExt);
	inputFormatMap().add(ret);
	return ret;
      }();
      return format;
    }

    //! Register a type with the format.
    template<typename ObjectT>
    static bool registerType()
    {
      return format()->template registerObjectType<ObjectT>();
    }

    //! See if we know how to read the stream.
    [[nodiscard]] std::optional<StreamInputPlan> probe(const ProbeInputContext &ctx) final
    {
      if(ctx.m_data.size() < 4) {
        return std::nullopt;
      }
      std::string initialData = std::string(ctx.m_data.begin(), ctx.m_data.end());
      // Try and read the header.
      std::istringstream ss(initialData);
      ArchiveT archive(ss);
      CerealArchiveHeader header;
      try {
	archive(header);
      } catch (const std::exception &e) {
	// If we try and read the header and the magic number is
	// not there, we can't read the stream.
	return std::nullopt;
      }
      if(header.m_magic != 0xABBA2024) {
	return std::nullopt;
      }
      if(header.fileFormatVersion != 1) {
	SPDLOG_WARN("Version mismatch in stream. Expected 1 got {}", header.fileFormatVersion);
	return std::nullopt;
      }
      // We need a lock now, as we are accessing the factory.
      std::shared_lock lock(m_mutex);
      auto it = m_streamInputFactory.find(header.typeName);
      if(it == m_streamInputFactory.end()) {
	SPDLOG_WARN("Unknown object type '{}' in format '{}' Archive:{} ", header.typeName, typeName(typeid(*this)),static_cast<void*>(this));
        for(const auto &pair : m_streamInputFactory) {
          SPDLOG_INFO("  Known type: '{}'", pair.first);
        }
	return std::nullopt;
      }
      auto newStream = it->second(ctx);
      if(ctx.m_targetType ==newStream->type()) {
        return StreamInputPlan {newStream, {}, 1.0f};
      }
      std::optional<ConversionChain> conv = typeConverterMap().find(ctx.m_targetType, newStream->type());
      if(!conv.has_value()) {
	SPDLOG_WARN("Don't know how to convert from {} to {}", newStream->type().name(), ctx.m_targetType.name());
	return std::nullopt;
      }
      return StreamInputPlan {newStream, conv.value(), 1.0f};
    }
  private:
    std::shared_mutex m_mutex;
    //! Factory function for creating the stream.
    std::unordered_map<std::string,std::function<std::shared_ptr<StreamInputBase>(const ProbeInputContext &ctx)> > m_streamInputFactory;
  };

  //! Make sure these aren't instantiated in every translation unit.
  extern template class CerealSaveFormat<cereal::BinaryOutputArchive>;
  extern template class CerealLoadFormat<cereal::BinaryInputArchive>;
  extern template class CerealSaveFormat<cereal::JSONOutputArchive>;
  extern template class CerealLoadFormat<cereal::JSONInputArchive>;

  //! @brief Register a type with the binary cereal formats.
  template<typename ObjectT>
  bool registerCerealFormats()
  {
    bool ret = CerealSaveFormat<cereal::BinaryOutputArchive>::template registerType<ObjectT>();
    ret &= CerealLoadFormat<cereal::BinaryInputArchive>::template registerType<ObjectT>();
    return ret;
  }

  //! @brief Register a type with the cereal json formats.
  template<typename ObjectT>
  bool registerCerealJSONFormats()
  {
    bool ret = CerealSaveFormat<cereal::JSONOutputArchive>::template registerType<ObjectT>();
    ret &= CerealLoadFormat<cereal::JSONInputArchive>::template registerType<ObjectT>();
    return ret;
  }

}// namespace Ravl2
