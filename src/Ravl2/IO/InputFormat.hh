//
// Created by charles on 24/08/24.
//

#pragma once

#include <typeinfo>
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <shared_mutex>
#include <istream>
#include <functional>
#include <nlohmann/json.hpp>
#include <utility>
#include "Ravl2/IO/StreamInput.hh"
#include "Ravl2/IO/TypeConverter.hh"

namespace Ravl2
{

  //! Information used when probing possible input formats for a given URL.
  //! This context is constructed by the IO layer and passed to each registered
  //! InputFormat implementation so it can decide whether it can read the source
  //! and, if so, return a StreamInputPlan describing how to load it.
  //!
  //! The key design goal is to allow format handlers to make an informed choice
  //! based on:
  //!  - The requested target C++ type (m_targetType)
  //!  - The URL broken into protocol, filename, and extension
  //!  - Optional JSON hints (m_formatHint) such as verbosity or desired
  //!    sub-features, without baking policy into handlers
  //!  - Optionally an already-open stream and a small look‑ahead buffer (m_data)
  //!
  //! Thread-safety: The context is immutable after construction, except for
  //! mStream which may be populated by the caller to share an already-open
  //! stream. Handlers must treat the context as read-only. If a handler needs
  //! to maintain state for the eventual decode, it should capture that state
  //! inside the StreamInputPlan it returns (e.g., via a shared_ptr to an RAII
  //! decode context).
  //!
  //! Lifetime and ownership:
  //!  - mStream is a shared_ptr managed by the caller; handlers must not reset
  //!    or close it.
  //!  - m_data is a copy of any look‑ahead bytes provided by the caller and can
  //!    be safely inspected by handlers for signature matching.
  //!  - m_targetType is a reference valid for the duration of the probe call.
  //!
  //! @example
  //! @code
  //! auto plan = inputFormatMap().probe(ProbeInputContext{
  //!   url, filename, "file", ext, defaultLoadFormatHint(), typeid(Array<PixelRGB8,2>)
  //! });
  //! if (plan) { /* use plan->mStream and plan->mConversion to read */ }
  //! @endcode

  class ProbeInputContext
  {
  public:
    //! Construct a probe context
    //! @param url Full URL (e.g., file:///path/to/image.jpg or camera:0)
    //! @param filename Resource identifier within the protocol (often a path)
    //! @param protocol Protocol selector ("file", "camera", "http", ...)
    //! @param ext Lowercase extension without dot used for handler lookup
    //! @param formatHint Free-form JSON hint map. Recognised keys include:
    //!   - "verbose" (bool): emit INFO logs from probing/selection.
    //! @param targetType The desired C++ type to ultimately produce. The
    //!   probing code will prefer plans that decode natively close to this
    //!   type and then use the TypeConverter system to finish the conversion.
    ProbeInputContext(std::string url, std::string filename, std::string protocol, std::string ext, nlohmann::json formatHint, const std::type_info &targetType)
        : m_url(std::move(url)),
          mFilename(std::move(filename)),
          m_protocol(std::move(protocol)),
          m_extension(std::move(ext)),
          m_formatHint(std::move(formatHint)),
          m_targetType(targetType)
    {}

    //! Optional pre-opened input stream for handlers that can consume std::istream.
    //! If provided, handlers should read cautiously (respecting current position)
    //! and not assume seekability unless documented by the caller.
    std::shared_ptr<std::istream> mStream;
    //! Original URL provided by the user (scheme + path or opaque id)
    std::string m_url;
    //! Filename or resource id portion of the URL (often a filesystem path)
    std::string mFilename;
    //! Access protocol (e.g., "file", "camera", "http"). Used to filter handlers.
    std::string m_protocol;
    //! Lowercase file extension (without the leading dot). Used to select candidate handlers.
    std::string m_extension;
    //! Free-form JSON hints affecting probing/decoding policy.
    nlohmann::json m_formatHint;
    //! Target C++ type to be produced by the final conversion chain.
    const std::type_info &m_targetType;
    //! Optional look-ahead bytes from the beginning of the resource. Handlers
    //! may use this to match magic values without touching mStream.
    std::vector<uint8_t> m_data;
    //! Convenience flag mirrored from m_formatHint["verbose"]. If true, handlers
    //! should emit SPDLOG_INFO messages for state transitions.
    bool mVerbose = false;
  };

  //! Abstract base class describing an input file/stream format.
  //! Implementations register themselves in the global InputFormatMap and are
  //! queried during probing. Each handler declares the protocol and extension
  //! it is interested in, and a priority. When multiple handlers match a given
  //! (protocol, extension), higher priority handlers are probed first.

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

    //! @brief Ask the handler to build a plan for reading this source.
    //! Implementations should inspect ctx and decide if they can handle
    //! the source. If so, they return a StreamInputPlan that provides a
    //! StreamInput<T> and a TypeConverter chain to the requested target.
    //! If the format is not recognised, return std::nullopt.
    //! Handlers should avoid expensive work during probe; prefer to defer
    //! actual decoding until the stream is first read.
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
  //! This is a convenience implementation of InputFormat that forwards
  //! probe() to a stored callable.
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

  //! @brief Registry mapping extensions to one or more InputFormat handlers.
  //! Handlers register themselves into this map. Probing looks up the list of
  //! handlers associated with ctx.m_extension (or the default "" bucket) and
  //! calls them in decreasing priority order until one returns a plan.

  class InputFormatMap
  {
  public:
    InputFormatMap() = default;
    
    InputFormatMap(const InputFormatMap &) = delete;
    InputFormatMap &operator=(const InputFormatMap &) = delete;
    InputFormatMap(InputFormatMap &&) = delete;
    InputFormatMap &operator=(InputFormatMap &&) = delete;
    
    //! Add a format to the map.
    //! @param format The format to add.
    //! @return True if the format was added.
    bool add(std::shared_ptr<InputFormat> format);

    //! Probe available formats for a plan matching the given context.
    //! Handlers that do not match the protocol or cannot recognise the source
    //! should return std::nullopt. The first successful plan is returned.
    [[nodiscard]] std::optional<StreamInputPlan> probe(const ProbeInputContext &ctx);

  private:
    std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::vector<std::shared_ptr<InputFormat>>> m_formatByExtension;
  };

  //! Access the global input format registry (singleton).
  [[nodiscard]] InputFormatMap &inputFormatMap();

}// namespace Ravl2