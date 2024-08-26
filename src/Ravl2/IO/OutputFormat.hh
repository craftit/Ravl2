
#pragma once

#include <typeinfo>
#include <nlohmann/json.hpp>
#include <utility>
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/StreamOutput.hh"

namespace Ravl2
{

  //! Information about a file we're saving.

  class ProbeOutputContext
  {
  public:
    ProbeOutputContext(std::string url, const std::string &filename, std::string protocol, std::string ext, nlohmann::json formatHint,const std::type_info &sourceType)
      : m_url(std::move(url)),
        m_filename(filename),
        m_protocol(std::move(protocol)),
        m_extension(std::move(ext)),
        m_formatHint(std::move(formatHint)),
        m_sourceType(sourceType)
    {}

    std::string m_url; //! The URL of the file.
    std::string m_filename; //! The name of the file, with the extension but without the protocol.
    std::string m_protocol; //! The protocol used to save the file. http, file, etc.
    std::string m_extension;
    nlohmann::json m_formatHint;
    const std::type_info &m_sourceType;
    bool m_verbose = false;
  };


  //! Save file format
  //! We select the format to use on save based on the file extension, and the data type.
  //! The user may also provide json to specify the format.

  class OutputFormat
  {
  public:

    //! Default constructor.
    OutputFormat() = default;

    //! Constructor.
    OutputFormat(std::string format, std::string extension, std::string protocol,int priority = 0)
      : m_name(std::move(format)),
        m_extension(std::move(extension)),
        m_protocol(std::move(protocol)),
        m_priority(priority)
    {}

    //! Virtual destructor.
    virtual ~OutputFormat() = default;

    //! Get the format name.
    [[nodiscard]] const std::string &name() const noexcept
    {
      return m_name;
    }

    //! Get the extension.
    [[nodiscard]] const std::string &extension() const noexcept
    {
      return m_extension;
    }

    //! Get the protocol.
    [[nodiscard]] const std::string &protocol() const noexcept
    {
      return m_protocol;
    }

    //! Get the priority of the format.
    [[nodiscard]] int priority() const noexcept
    {
      return m_priority;
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

    //! Test if we can save this type.
    [[nodiscard]] virtual std::optional<StreamOutputPlan> probe(const ProbeOutputContext &ctx) const = 0;

  private:
    std::string m_name;
    std::string m_extension; //!< Extension of the file, maybe a comma separated list.
    std::string m_protocol; //!< Protocol of the file.
    int m_priority = 0;
  };

  //! @brief Construct a format handler from a callback.

  class OutputFormatCall
    : public OutputFormat
  {
  public:
    //! @brief Construct a format handler from a callback.
    //! @param format - The format of the file.
    //! @param extension - The extension of the file. - If empty, the format handles multiple extensions, maybe a comma separated list.
    //! @param callback - The callback to write the object.
    OutputFormatCall(std::string name, std::string extension, std::string protocol, int priority, std::function<std::optional<StreamOutputPlan>(const ProbeOutputContext &)> callback)
      : OutputFormat(std::move(name), std::move(extension), protocol, priority),
        m_callback(std::move(callback))
    {}

    //! Test if we can save this type.
    [[nodiscard]] std::optional<StreamOutputPlan> probe(const ProbeOutputContext &ctx) const override
    {
      return m_callback(ctx);
    }

  private:
    std::function<std::optional<StreamOutputPlan>(const ProbeOutputContext &)> m_callback;
  };

  //! @brief Map of save formats.
  //! This class is used to map file extensions to save formats.

  class OutputFormatMap
  {
  public:
    //! Add a format to the map.
    //! @param format - The format to add.
    //! @return True if the format was added.
    bool add(std::shared_ptr<OutputFormat> format);

    //! Get the format for a given extension.
    [[nodiscard]] std::optional<StreamOutputPlan> probe(const ProbeOutputContext &ctx);

  private:
    std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::vector<std::shared_ptr<OutputFormat> > > m_formatByExtension;
  };

  OutputFormatMap &outputFormatMap();


}// namespace Ravl2
