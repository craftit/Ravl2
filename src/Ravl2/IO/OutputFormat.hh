
#pragma once

#include <typeinfo>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <shared_mutex>
#include <functional>
#include <nlohmann/json.hpp>
#include <utility>
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/IO/StreamOutput.hh"

namespace Ravl2
{

  //! Information used when probing possible output formats for a given URL.
  //! This context is constructed by the IO layer and passed to each registered
  //! OutputFormat implementation so it can decide whether it can write the
  //! provided source type to the destination URL, and, if so, return a
  //! StreamOutputPlan describing how to save it.
  //!
  //! Goals and usage:
  //!  - Allow format handlers to make an informed choice based on the
  //!    destination (protocol, filename, extension), the source C++ type to be
  //!    written (m_sourceType), and optional JSON hints (m_formatHint).
  //!  - Handlers should treat the context as immutable and must not modify it.
  //!    If a handler needs state for the eventual write, capture it inside the
  //!    StreamOutputPlan it returns (e.g., via a shared_ptr to an RAII encoder
  //!    context).
  //!
  //! Ownership and lifetime:
  //!  - Strings and JSON are owned by the context and are valid for the probe
  //!    call duration. Handlers should copy out what they need to keep.
  //!  - m_sourceType is a reference valid during the probe call only.
  //!
  //! Thread-safety: The context is read-only after construction. Format
  //! implementations must not write to any fields.
  //!
  //! @example
  //! @code
  //! // Selecting a writer and building a plan
  //! ProbeOutputContext octx{
  //!   url,
  //!   filename,
  //!   "file",
  //!   ext, // lowercase extension without dot
  //!   defaultSaveFormatHint(),
  //!   typeid(Array<PixelRGB8,2>)
  //! };
  //! auto plan = outputFormatMap().probe(octx);
  //! if (plan) {
  //!   // plan->mStream is a StreamOutput<T> for some ViaT the handler supports
  //!   // plan->mConversion (optional) converts from our source type to ViaT
  //!   // plan->mLoss reports the overall preserved-bits score
  //! }
  //! @endcode
  //!
  //! Recognised formatHint keys (conventions):
  //!  - "verbose" (bool): emit INFO logs from probing/selection and save path.
  //!  - Format-specific keys may be interpreted by individual handlers (e.g.,
  //!    JPEG quality, PNG compression), but should be documented by that
  //!    handler.
  class ProbeOutputContext
  {
  public:
    ProbeOutputContext(std::string url, const std::string &filename, std::string protocol, std::string ext, nlohmann::json formatHint, const std::type_info &sourceType)
        : m_url(std::move(url)),
          m_filename(filename),
          m_protocol(std::move(protocol)),
          m_extension(std::move(ext)),
          m_formatHint(std::move(formatHint)),
          m_sourceType(sourceType)
    {}

    //! Original URL provided by the user (scheme + path or opaque id)
    std::string m_url;
    //! Filename or resource id portion of the URL (often a filesystem path)
    std::string m_filename;
    //! Access protocol (e.g., "file", "http"). Used to filter handlers.
    std::string m_protocol;
    //! Lowercase extension (without leading dot). Used to select candidate handlers.
    std::string m_extension;
    //! Free-form JSON hints affecting probing/encoding policy.
    nlohmann::json m_formatHint;
    //! Source C++ type to be saved. Handlers find a conversion chain from this
    //! type to their preferred ViaT using the TypeConverter system.
    const std::type_info &m_sourceType;
    //! Convenience flag mirrored from m_formatHint["verbose"]. If true, handlers
    //! should emit SPDLOG_INFO messages for state transitions.
    bool m_verbose = false;
  };

  //! Abstract base class describing an output (save) format.
  //! A format declares a protocol and one or more extensions it can handle, and
  //! a priority. During saving, OutputFormatMap looks up handlers for the given
  //! extension and calls probe() for each until one returns a plan.
  //!
  //! Notes:
  //!  - Implementations should do minimal work in probe and defer actual
  //!    encoding to the returned StreamOutput<T>.
  //!  - Priority is available for selection policies; current map iterates in
  //!    insertion order.
  //!  - Handlers should not log the same error at multiple layers; log at the
  //!    boundary where action is taken (e.g., emit WARN when a format is not
  //!    applicable, ERROR when encoding fails inside the stream).

  class OutputFormat
  {
  public:
    //! Default constructor.
    OutputFormat() = default;

    //! Constructor.
    OutputFormat(std::string format, std::string extension, std::string protocol, int priority = 0)
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

    //! @brief Ask the handler to build a plan for writing to this destination.
    //! Implementations should inspect ctx and decide if they can handle the
    //! destination and source type. If so, they return a StreamOutputPlan that
    //! provides a StreamOutput<ViaT> and a TypeConverter chain from ctx.m_sourceType
    //! to ViaT. If the format is not recognised/applicable, return std::nullopt.
    [[nodiscard]] virtual std::optional<StreamOutputPlan> probe(const ProbeOutputContext &ctx) = 0;

  protected:
    //! Set the priority of the format.
    void setPriority(int priority)
    {
      m_priority = priority;
    }
  private:
    std::string m_name;
    std::string m_extension;//!< Extension of the file, maybe a comma separated list.
    std::string m_protocol; //!< Protocol of the file.
    int m_priority = 0;
  };

  //! @brief Convenience wrapper to implement an OutputFormat from a callback.

  class OutputFormatCall : public OutputFormat
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

    //! Forward probe() to the stored callback.
    [[nodiscard]] std::optional<StreamOutputPlan> probe(const ProbeOutputContext &ctx) override
    {
      return m_callback(ctx);
    }

  private:
    std::function<std::optional<StreamOutputPlan>(const ProbeOutputContext &)> m_callback;
  };

  //! @brief Registry mapping extensions to one or more OutputFormat handlers.
  //! Handlers register themselves into this map. Probing looks up the list of
  //! handlers associated with ctx.m_extension (or the default "" bucket) and
  //! calls them in insertion order until one returns a plan.

  class OutputFormatMap
  {
  public:
    OutputFormatMap() = default;
   
    //! Delete copy constructor.
    OutputFormatMap(const OutputFormatMap &) = delete;
    OutputFormatMap &operator=(const OutputFormatMap &) = delete;
    OutputFormatMap(OutputFormatMap &&) = delete;
    OutputFormatMap &operator=(OutputFormatMap &&) = delete;
    
    //! Add a format to the map.
    //! @param format - The format to add.
    //! @return True if the format was added.
    bool add(std::shared_ptr<OutputFormat> format);

    //! Get the format for a given extension.
    [[nodiscard]] std::optional<StreamOutputPlan> probe(const ProbeOutputContext &ctx);

  private:
    std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::vector<std::shared_ptr<OutputFormat>>> m_formatByExtension;
  };

  [[nodiscard]] OutputFormatMap &outputFormatMap();

}// namespace Ravl2
