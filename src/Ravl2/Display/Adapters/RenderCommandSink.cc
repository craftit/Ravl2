#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

#include "Ravl2/Array.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/Display/DebugDisplay.hh"
#include "Ravl2/Display/IRenderCommand.hh"
#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/Commands/SetBaseImage2D.hh"
#include "Ravl2/Display/Commands/SetNormalization2D.hh"

namespace Ravl2::DebugDisplay {
  void initDisplay()
  {}
namespace {

// Parse @debug URL of the form "@debug:Channel[:Control[:Control...]]"
struct DebugUrlParse {
  std::string channel;
  std::string controls; // concatenated suffix starting from first ':' after channel
};

static std::optional<DebugUrlParse> parseDebugUrl(const std::string &url) {
  constexpr std::string_view kPrefix = "@debug:";
  if (url.rfind(kPrefix.data(), 0) != 0) return std::nullopt;
  DebugUrlParse out;
  std::string rest = url.substr(kPrefix.size());
  // Channel is up to next ':' or end
  auto pos = rest.find(':');
  if (pos == std::string::npos) {
    out.channel = rest;
    out.controls.clear();
  } else {
    out.channel = rest.substr(0, pos);
    out.controls = rest.substr(pos); // includes ':'
  }
  return out;
}

// Local clear command kept internal to avoid using deprecated shim
struct ClearChannelCommand : public IRenderCommand {
  std::string channel;
  explicit ClearChannelCommand(std::string ch) : channel(std::move(ch)) {}
  void apply(ChannelRegistry &channels) override {
    channels.clearChannel(channel);
    SPDLOG_INFO("DebugDisplay: cleared channel '{}'", channel);
  }
};

// Converters: Array<uint8_t,2> -> shared_ptr<IRenderCommand>
static std::shared_ptr<IRenderCommand> makeCmdFromU8Array(const Array<uint8_t,2> &img)
{
  auto cmd = std::make_shared<SetBaseImage2D>(std::string{} /*channel set by sink from URL*/);
  cmd->width = img.range()[1].size();
  cmd->height = img.range()[0].size();
  cmd->isFloat = false;
  cmd->u8.resize(static_cast<size_t>(cmd->width) * static_cast<size_t>(cmd->height));
  for (int y=0; y<cmd->height; ++y) {
    for (int x=0; x<cmd->width; ++x) {
      cmd->u8[size_t(y)*size_t(cmd->width) + size_t(x)] = img[{y, x}];
    }
  }
  return cmd;
}

// Converters: Array<float,2> -> shared_ptr<IRenderCommand>
static std::shared_ptr<IRenderCommand> makeCmdFromF32Array(const Array<float,2> &img)
{
  auto cmd = std::make_shared<SetBaseImage2D>(std::string{} /*channel set by sink from URL*/);
  cmd->width = img.range()[1].size();
  cmd->height = img.range()[0].size();
  cmd->isFloat = true;
  cmd->f32.resize(static_cast<size_t>(cmd->width) * static_cast<size_t>(cmd->height));
  for (int y=0; y<cmd->height; ++y) {
    for (int x=0; x<cmd->width; ++x) {
      cmd->f32[size_t(y)*size_t(cmd->width) + size_t(x)] = img[{y, x}];
    }
  }
  return cmd;
}

// Register type conversions when this TU is loaded.
[[maybe_unused]] bool g_registerConverters = [](){
  SPDLOG_INFO("Registering TypeConverter: Array<u8,2>/Array<f32,2> -> shared_ptr<IRenderCommand>");
  bool ok1 = registerConversion(makeCmdFromU8Array, 1.0f);
  bool ok2 = registerConversion(makeCmdFromF32Array, 0.95f);
  (void)ok1; (void)ok2;
  return true;
}();

struct OutputFormatDebugDisplayCmdSink : public Ravl2::OutputFormat {
  OutputFormatDebugDisplayCmdSink() : OutputFormat("DebugDisplayCmdSink", "", "", 1100) {}

  static std::optional<NormalizationSettings> parseNormControls(const std::string &controls) {
    if (controls.empty()) return std::nullopt;
    // Accept :Norm=Auto | :Normalize=Auto
    auto pos = controls.find(":Norm=");
    size_t keyLen = 6; // len(":Norm=")
    if (pos == std::string::npos) {
      pos = controls.find(":Normalize=");
      keyLen = 11; // len(":Normalize=")
    }
    if (pos == std::string::npos) return std::nullopt;
    auto val = controls.substr(pos + keyLen);
    // Clip at next ':'
    auto colon = val.find(':');
    if (colon != std::string::npos) val = val.substr(0, colon);

    NormalizationSettings ns{};
    if (val == "Auto") {
      ns.policy = NormalizationPolicy::Auto;
      return ns;
    }
    // Fixed:min,max
    if (val.rfind("Fixed", 0) == 0) {
      ns.policy = NormalizationPolicy::Fixed;
      auto p = val.find('=');
      if (p != std::string::npos) {
        auto args = val.substr(p+1);
        auto comma = args.find(',');
        if (comma != std::string::npos) {
          try {
            ns.minVal = std::stof(args.substr(0, comma));
            ns.maxVal = std::stof(args.substr(comma+1));
          } catch(...) {}
        }
      }
      return ns;
    }
    // Pct:low,high or Percentile:low,high
    if (val.rfind("Pct", 0) == 0 || val.rfind("Percentile", 0) == 0) {
      ns.policy = NormalizationPolicy::Percentile;
      auto p = val.find(':');
      if (p != std::string::npos) {
        auto args = val.substr(p+1);
        auto comma = args.find(',');
        if (comma != std::string::npos) {
          try {
            ns.lowPct = std::stof(args.substr(0, comma));
            ns.highPct = std::stof(args.substr(comma+1));
          } catch(...) {}
        }
      }
      return ns;
    }
    return std::nullopt;
  }

  std::optional<StreamOutputPlan> probe(const ProbeOutputContext &ctx) override {
    auto parsed = parseDebugUrl(ctx.m_url);
    if (!parsed) return std::nullopt;

    // Target sink type: shared_ptr<IRenderCommand>
    using CmdPtr = std::shared_ptr<IRenderCommand>;

    std::optional<ConversionChain> conv;
    if (ctx.m_sourceType != typeid(CmdPtr)) {
      conv = typeConverterMap().find(typeid(CmdPtr), ctx.m_sourceType);
      if (!conv.has_value()) {
        return std::nullopt; // we only accept types convertible to CmdPtr
      }
    }

    auto stream = std::make_shared<StreamOutputCall<CmdPtr>>([url=ctx.m_url](const CmdPtr &cmd, std::streampos pos) -> std::streampos {
      (void)pos;
      auto parsed2 = parseDebugUrl(url);
      if (!parsed2) return std::streampos(0);

      DebugDisplay::ensureStarted({});

      // Handle :Clear first if present
      if (!parsed2->controls.empty() && parsed2->controls.find(":Clear") != std::string::npos) {
        auto clearCmd = std::make_shared<ClearChannelCommand>(parsed2->channel);
        DebugDisplay::enqueue(clearCmd);
      }

      // Handle normalization controls if present
      if (!parsed2->controls.empty()) {
        if (auto ns = parseNormControls(parsed2->controls)) {
          auto normCmd = std::make_shared<SetNormalization2D>(parsed2->channel, *ns);
          DebugDisplay::enqueue(normCmd);
        }
      }

      // If this is a SetBaseImage2D, enforce channel from URL
      if (cmd) {
        if (auto *setImg = dynamic_cast<SetBaseImage2D*>(cmd.get())) {
          setImg->channel = parsed2->channel;
        }
        // For other commands we just enqueue as-is.
        DebugDisplay::enqueue(cmd);
      }
      return std::streampos(0);
    });

    if (ctx.m_sourceType == typeid(CmdPtr)) {
      return StreamOutputPlan{.mStream = stream, .mConversion = {}, .mCost = 1.0f};
    }
    return StreamOutputPlan{.mStream = stream, .mConversion = conv.value(), .mCost = conv->conversionLoss()};
  }
};

[[maybe_unused]] bool g_registerCmdSink = [](){
  SPDLOG_INFO("Registering DebugDisplay @debug command sink (shared_ptr<IRenderCommand>)");
  return outputFormatMap().add(std::make_shared<OutputFormatDebugDisplayCmdSink>());
}();

} // namespace
} // namespace Ravl2::DebugDisplay
