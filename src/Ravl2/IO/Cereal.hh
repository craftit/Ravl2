//
// Created by charles galambos on 24/08/2024.
//

#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/InputFormat.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/IO/Load.hh"

namespace Ravl2
{

  struct CerealArchiveHeader
  {
    constexpr static uint32_t m_magicNumber = 0xABBA2024;
    CerealArchiveHeader() = default;
    explicit CerealArchiveHeader(const std::string &theTypeName)
      : m_magic(m_magicNumber),
        version(1),
        typeName(theTypeName)
    {}
    uint32_t m_magic = 0;
    uint16_t version = 0;
    std::string typeName;

    template <class Archive>
    void serialize(Archive &archive)
    {
      archive(m_magic, version, typeName);
    }
  };


  //! @brief Archive that uses cereal to write objects to a stream.
  //! @tparam ObjectT - The type of object to write.
  template <typename ObjectT, typename ArchiveT>
  class StreamOutputCerealArchive : public StreamOutputContainer<ObjectT>
  {
  public:
    explicit StreamOutputCerealArchive(std::unique_ptr<std::ostream> stream)
      : m_archive(*stream),
        m_stream(std::move(stream))
    {}

    std::streampos write(const ObjectT &obj, std::streampos pos) override
    {
      m_stream->seekp(pos);
      CerealArchiveHeader header(typeName(typeid(ObjectT)));
      m_archive(header);
      m_archive(obj);
      return m_stream->tellp();
    }

  private:
    ArchiveT m_archive;
    std::unique_ptr<std::ostream> m_stream;
  };

  //! @brief Archive that uses cereal to write objects to a stream.
  //! @tparam ObjectT - The type of object to write.
  template <typename ObjectT, typename ArchiveT>
  class StreamInputCerealArchive : public StreamInputContainer<ObjectT>
  {
  public:
    explicit StreamInputCerealArchive(std::unique_ptr<std::istream> stream)
        : m_archive(*stream),
          m_stream(std::move(stream))
    {}

    //! Goto next position in the stream and read the object.
    //! @param pos - The position in the stream where the object was written.
    //! @return The object.
    std::optional<ObjectT> next(std::streampos &pos) final
    {
      if (m_stream->eof())
        return std::nullopt;
      m_stream->seekg(pos);
      CerealArchiveHeader header;
      m_archive(header);
      if(header.m_magic != 0xABBA2024)
      {
        spdlog::error("Magic number mismatch in stream. Expected 0xABBA2024 got {}", header.m_magic);
        return std::nullopt;
      }
      if(header.version != 1)
      {
        spdlog::error("Version mismatch in stream. Expected 1 got {}", header.version);
        return std::nullopt;
      }
      if (header.typeName != typeName(typeid(ObjectT)))
      {
        spdlog::error("Type mismatch in stream. Expected {} got {}", typeName(typeid(ObjectT)), header.typeName);
        return std::nullopt;
      }
      ObjectT obj;
      m_archive(obj);
      pos = m_stream->tellg();
      return obj;
    }

  private:
    ArchiveT m_archive;
    std::unique_ptr<std::istream> m_stream;
  };

  //! @brief File format for saving objects to a binary file using cereal.
  template <typename ObjectT, typename ArchiveT>
  class CerealSaveFormat
   : public OutputFormat
  {
  public:
    CerealSaveFormat()
      : OutputFormat("CerealBin", "bin")
    {}

    [[nodiscard]] std::unique_ptr<StreamOutputBase> createStream(const ProbeOutputContext &context) const
    {
      auto stream = std::make_unique<std::ofstream>(context.m_filename, std::ios::binary);
      return std::make_unique<StreamOutputCerealArchive<ObjectT, ArchiveT>>(std::move(stream));
    }

    //! Test if we can save this type.
    [[nodiscard]] std::optional<StreamOutputPlan> probe(const ProbeOutputContext &ctx) const final
    {
      if(ctx.m_extension != this->extension()) {
        return std::nullopt;
      }
      if (ctx.m_sourceType == typeid(ObjectT))
      {
        return StreamOutputPlan {createStream(ctx), {},1.0f};
      }
      std::optional<ConversionChain> conv = typeConverterMap().find(typeid(ObjectT), ctx.m_sourceType);
      if (conv.has_value())
      {
        return StreamOutputPlan  {createStream(ctx), conv.value(), 1.0f};
      }
      return std::nullopt;
    }

  };

  //! @brief Cereal load format.
  template <typename ObjectT, typename ArchiveT>
  class CerealLoadFormat
      : public InputFormat
  {
  public:
    CerealLoadFormat()
        : InputFormat("CerealBin", "bin")
    {}

    [[nodiscard]] std::unique_ptr<StreamInputBase> createStream(const ProbeInputContext &context) const
    {
      auto stream = std::make_unique<std::ifstream>(context.m_filename, std::ios::binary);
      return std::make_unique<StreamInputCerealArchive<ObjectT, ArchiveT>>(std::move(stream));
    }

    [[nodiscard]] std::optional<StreamInputPlan> probe(const ProbeInputContext &ctx) const final
    {
      if(ctx.m_data.size() < 4)
      {
        return std::nullopt;
      }
      {
        std::string initialData = std::string(ctx.m_data.begin(), ctx.m_data.end());
        std::istringstream ss(initialData);
        ArchiveT archive(ss);
        std::string headerTypeName;
        uint16_t version = 0;
        archive(version, headerTypeName);
        if(headerTypeName != typeName(typeid(ObjectT)) || version != 1)
        {
          return std::nullopt;
        }
      }
      if(ctx.m_targetType == typeid(ObjectT))
      {
        return StreamInputPlan { createStream(ctx), {}, 1.0f };
      }
      return std::nullopt;
    }
  };


}

