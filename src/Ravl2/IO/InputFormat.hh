//
// Created by charles on 24/08/24.
//

#pragma once

#include <typeinfo>
#include <nlohmann/json.hpp>
#include <utility>
#include "Ravl2/IO/StreamInput.hh"
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2
{

  //! Information about a file we're loading.
  //! This includes the file name, and the type of data we're loading as well as the first few bytes of the file.

  class ProbeInputContext
  {
  public:
    ProbeInputContext(std::string url, std::string filename, std::string protocol, std::string ext, nlohmann::json formatHint, const std::type_info &targetType)
        : m_url(std::move(url)),
          m_filename(std::move(filename)),
          m_protocol(std::move(protocol)),
          m_extension(std::move(ext)),
          m_formatHint(std::move(formatHint)),
          m_targetType(targetType)
    {}

    std::shared_ptr<std::istream> mStream;
    std::string m_url;
    std::string m_filename;
    std::string m_protocol;
    std::string m_extension;
    nlohmann::json m_formatHint;
    const std::type_info &m_targetType;
    std::vector<uint8_t> m_data;
    bool m_verbose = false;
  };

  //! Load file format

  class InputFormat
  {
  public:
    InputFormat() = default;

    InputFormat(std::string name, std::string extension, std::string protocol, int priority = 0)
        : m_name(std::move(name)),
          m_extension(std::move(extension)),
          m_protocol(std::move(protocol)),
          m_priority(priority)
    {}

    //! Make destructor virtual
    virtual ~InputFormat() = default;

    //! Get the name of the format.
    [[nodiscard]] const std::string &name() const noexcept
    {
      return m_name;
    }

    //! Get the extension of the format.
    [[nodiscard]] const std::string &extension() const noexcept
    {
      return m_extension;
    }

    //! Get the protocol of the format.
    [[nodiscard]] const std::string &protocol() const noexcept
    {
      return m_protocol;
    }

    //! Test if we support a protocol.
    [[nodiscard]] bool supportsProtocol(const std::string &protocol) const noexcept
    {
      return m_protocol == protocol;
    }

    //! Test if we support an extension.
    [[nodiscard]] bool supportsExtension(const std::string &extension) const noexcept
    {
      return m_extension == extension;
    }

    //! @brief Test if we can load this type.
    //! @param ctx - The context to see if we can load.
    //! @return optional, if format is unknown, return nullopt, otherwise return data the format can use to load the file.
    [[nodiscard]] virtual std::optional<StreamInputPlan> probe(const ProbeInputContext &ctx) = 0;

    //! Get the priority of the format.
    [[nodiscard]] int priority() const noexcept
    {
      return m_priority;
    }

  private:
    std::string m_name;
    std::string m_extension;
    std::string m_protocol;
    int m_priority = 0;
  };

  //! Wrapper for a function or lambda that can probe a file format.
  class InputFormatCall : public InputFormat
  {
  public:
    //! Constructor
    //! @param name - The name of the format.
    //! @param extension - Expected extension of the format.
    //! @param priority - Priority of the format. Higher is better, default is 0.
    InputFormatCall(std::string name, std::string extension, std::string protocol, int priority, std::function<std::optional<StreamInputPlan>(const ProbeInputContext &)> probe)
        : InputFormat(std::move(name), std::move(extension), std::move(protocol), priority),
          m_probe(std::move(probe))
    {}

    //! @brief Test if we can load this type.
    //! @param ctx - The context to see if we can load.
    //! @return optional, if format is unknown, return nullopt, otherwise return data the format can use to load the file.
    [[nodiscard]] std::optional<StreamInputPlan> probe(const ProbeInputContext &ctx) final
    {
      return m_probe(ctx);
    }

  private:
    std::function<std::optional<StreamInputPlan>(const ProbeInputContext &)> m_probe;
  };

  //! @brief Map of save formats.
  //! This class is used to map file extensions to save formats.

  class InputFormatMap
  {
  public:
    InputFormatMap() = default;
    
    InputFormatMap(const InputFormatMap &) = delete;
    InputFormatMap &operator=(const InputFormatMap &) = delete;
    InputFormatMap(InputFormatMap &&) = delete;
    InputFormatMap &operator=(InputFormatMap &&) = delete;
    
    //! Add a format to the map.
    //! @param format - The format to add.
    //! @return True if the format was added.
    bool add(std::shared_ptr<InputFormat> format);

    //! Get the format for a given extension.
    [[nodiscard]] std::optional<StreamInputPlan> probe(const ProbeInputContext &ctx);

  private:
    std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::vector<std::shared_ptr<InputFormat>>> m_formatByExtension;
  };

  [[nodiscard]] InputFormatMap &inputFormatMap();

}// namespace Ravl2