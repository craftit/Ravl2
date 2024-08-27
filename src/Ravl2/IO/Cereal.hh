//
// Created by charles galambos on 24/08/2024.
//

#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <utility>
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/IO/Load.hh"

namespace Ravl2
{

  struct CerealArchiveHeader {
    constexpr static uint32_t m_magicNumber = 0xABBA2024;
    CerealArchiveHeader() = default;
    explicit CerealArchiveHeader(std::string theTypeName)
        : m_magic(m_magicNumber),
          version(1),
          typeName(std::move(theTypeName))
    {}
    uint32_t m_magic = 0;
    uint16_t version = 0;
    std::string typeName;

    template<class Archive>
    void save(Archive & archive) const
    {
      archive(m_magic, version, typeName);
    }

    template<class Archive>
    void load(Archive & archive)
    {
      archive(m_magic);
      if(m_magic != m_magicNumber) {
	throw std::runtime_error("Magic number mismatch in stream.");
      }
      // We should check the version here too.
      archive(version, typeName);
    }
  };

  //! @brief Archive that uses cereal to write objects to a stream.
  //! @tparam ObjectT - The type of object to write.
  template <typename ObjectT, typename ArchiveT>
  class StreamOutputCerealArchive : public StreamOutput<ObjectT>
  {
  public:
    explicit StreamOutputCerealArchive(std::shared_ptr<std::ostream> stream)
    : m_archive(*stream),
      m_stream(std::move(stream))
    {}

    std::streampos write(const ObjectT &obj, std::streampos pos) override
    {
      if(m_first) {
	// Write the header.
	m_stream->seekp(0);
	m_archive(CerealArchiveHeader(typeName(typeid(ObjectT))));
	m_first = false;
	// Update the start position.
	this->mStart = m_stream->tellp();
      }
      if(pos != std::numeric_limits<std::streampos>::max()) {
	m_stream->seekp(pos);
      }
      m_archive(obj);
      auto at = m_stream->tellp();
      if(at > this->mEnd) {
	this->mEnd = at;
      }
      return at;
    }

  private:
    bool m_first = true;
    ArchiveT m_archive;
    std::shared_ptr<std::ostream> m_stream;
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
      m_stream->seekg(pos);
      CerealArchiveHeader header;
      ObjectT obj;
      m_archive(obj);
      pos = m_stream->tellg();
      return obj;
    }

  private:
    ArchiveT m_archive;
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

    template<typename ObjectT>
    static bool registerType()
    {
      static std::shared_ptr<CerealSaveFormat<ArchiveT> > format = []()
      {
	auto ret = std::make_shared<CerealSaveFormat<ArchiveT>>();
	outputFormatMap().add(ret);
	return ret;
      }();
      return format->template registerObjectType<ObjectT>();
    };

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
    explicit CerealLoadFormat(std::string ext = "bin")
      : InputFormat(fmt::format("Cereal-{}", typeName(typeid(ArchiveT))), ext, "file")
    {}

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

    template<typename ObjectT>
    static bool registerType()
    {
      static std::shared_ptr<CerealLoadFormat<ArchiveT> > format = []() {
	auto ret = std::make_shared<CerealLoadFormat<ArchiveT>>();
	inputFormatMap().add(ret);
	return ret;
      }();
      return format->template registerObjectType<ObjectT>();
    }

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
      if(header.version != 1) {
	SPDLOG_WARN("Version mismatch in stream. Expected 1 got {}", header.version);
	return std::nullopt;
      }
      // We need a lock now, as we are accessing the factory.
      std::shared_lock lock(m_mutex);
      auto it = m_streamInputFactory.find(header.typeName);
      if(it == m_streamInputFactory.end()) {
	SPDLOG_WARN("Unknown object type {}", header.typeName);
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

  //! Make sure these arn't instantiated in every translation unit.
  extern template class CerealSaveFormat<cereal::BinaryOutputArchive>;
  extern template class CerealLoadFormat<cereal::BinaryInputArchive>;
  extern template class CerealSaveFormat<cereal::JSONOutputArchive>;
  extern template class CerealLoadFormat<cereal::JSONInputArchive>;

  //! @brief Register a type with the cereal formats.
  template<typename ObjectT>
  bool registerCerealFormats()
  {
    bool ret = CerealSaveFormat<cereal::BinaryOutputArchive>::template registerType<ObjectT>();
    ret &= CerealLoadFormat<cereal::BinaryInputArchive>::template registerType<ObjectT>();
    ret &= CerealSaveFormat<cereal::JSONOutputArchive>::template registerType<ObjectT>();
    ret &= CerealLoadFormat<cereal::JSONInputArchive>::template registerType<ObjectT>();
    return ret;
  }

}// namespace Ravl2
