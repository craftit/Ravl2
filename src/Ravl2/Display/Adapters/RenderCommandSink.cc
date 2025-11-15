#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>
#include <SDL2/SDL.h>

#include "Ravl2/Array.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/TypeConverter.hh"
#include "Ravl2/Display/DebugDisplay.hh"
#include "Ravl2/Display/IRenderCommand.hh"
#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/Commands/SetBaseImage2D.hh"
#include "Ravl2/Display/Commands/SetNormalization2D.hh"
#include "Ravl2/Display/Commands/AddPolylineOverlay2D.hh"
#include "Ravl2/Geometry/PolyLine.hh"
#include "Ravl2/Geometry/PointSet.hh"
#include "Ravl2/Types.hh"
#include "Ravl2/Display/Commands/SetPointCloud3D.hh"
#include "Ravl2/Pixel/Pixel.hh"

namespace Ravl2::DebugDisplay
{
  void initDisplay()
  {
    // In headless mode we do not initialize SDL at all.
    if (isHeadless()) {
      SPDLOG_INFO("DebugDisplay: headless mode — skipping SDL initialization");
      return;
    }
    // Initialize SDL on the main thread (required for macOS)
    // macOS requires SDL initialization, particularly for window/menu system, on main thread
#ifdef __APPLE__
    // On macOS, set hint to reduce restrictions on secondary thread window creation
    SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "0");
#endif
    
    if (SDL_WasInit(0) == 0) {
      if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SPDLOG_ERROR("DebugDisplay: SDL_Init failed on main thread: {}", SDL_GetError());
        return;
      }
      SPDLOG_INFO("DebugDisplay: SDL initialized on main thread");
    }
  }

namespace {

// Parse @debug URL of the form "@debug:Channel[:Control[:Control...]]"
struct DebugUrlParse {
  std::string channel;
  std::string controls; // concatenated suffix starting from first ':' after channel
};

std::optional<DebugUrlParse> parseDisplayUrl(const std::string &url) {
  constexpr std::string_view kPrefix = "display://";
  SPDLOG_INFO("Parsing url: '{}' ",url);
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

std::shared_ptr<IRenderCommand> makeCmdFromU8Array(const Array<uint8_t,2> &img)
{
  auto cmd = std::make_shared<SetBaseImage2D_U8>(std::string{} /*channel set by sink from URL*/);
  cmd->width = img.range()[1].size();
  cmd->height = img.range()[0].size();
  cmd->data.resize(static_cast<size_t>(cmd->width) * static_cast<size_t>(cmd->height));
  for (int y=0; y<cmd->height; ++y) {
    for (int x=0; x<cmd->width; ++x) {
      cmd->data[size_t(y)*size_t(cmd->width) + size_t(x)] = img[{y, x}];
    }
  }
  return cmd;
}

// Converters: Array<float,2> -> shared_ptr<IRenderCommand>
std::shared_ptr<IRenderCommand> makeCmdFromF32Array(const Array<float,2> &img)
{
  auto cmd = std::make_shared<SetBaseImage2D_F32>(std::string{} /*channel set by sink from URL*/);
  cmd->width = img.range()[1].size();
  cmd->height = img.range()[0].size();
  cmd->data.resize(static_cast<size_t>(cmd->width) * static_cast<size_t>(cmd->height));
  for (int y=0; y<cmd->height; ++y) {
    for (int x=0; x<cmd->width; ++x) {
      cmd->data[size_t(y)*size_t(cmd->width) + size_t(x)] = img[{y, x}];
    }
  }
  return cmd;
}

// Converter: Array<PixelRGB8,2> -> shared_ptr<IRenderCommand>
 std::shared_ptr<IRenderCommand> makeCmdFromRGB8Array(const Array<PixelRGB8,2> &img)
{
  auto cmd = std::make_shared<SetBaseImage2D_RGB8>(std::string{} /*channel set by sink from URL*/);
  cmd->width = img.range()[1].size();
  cmd->height = img.range()[0].size();
  cmd->data.resize(static_cast<size_t>(cmd->width) * static_cast<size_t>(cmd->height));
  for (int y=0; y<cmd->height; ++y) {
    for (int x=0; x<cmd->width; ++x) {
      cmd->data[size_t(y)*size_t(cmd->width) + size_t(x)] = img[{y, x}];
    }
  }
  return cmd;
}

// Converter: Array<int16_t,2> -> shared_ptr<IRenderCommand>
 std::shared_ptr<IRenderCommand> makeCmdFromI16Array(const Array<int16_t,2> &img)
{
  auto cmd = std::make_shared<SetBaseImage2D_I16>(std::string{} /*channel set by sink from URL*/);
  cmd->width = img.range()[1].size();
  cmd->height = img.range()[0].size();
  cmd->data.resize(static_cast<size_t>(cmd->width) * static_cast<size_t>(cmd->height));
  for (int y=0; y<cmd->height; ++y) {
    for (int x=0; x<cmd->width; ++x) {
      cmd->data[size_t(y)*size_t(cmd->width) + size_t(x)] = img[{y, x}];
    }
  }
  return cmd;
}

// Converter: Array<int32_t,2> -> shared_ptr<IRenderCommand>
 std::shared_ptr<IRenderCommand> makeCmdFromI32Array(const Array<int32_t,2> &img)
{
  auto cmd = std::make_shared<SetBaseImage2D_I32>(std::string{} /*channel set by sink from URL*/);
  cmd->width = img.range()[1].size();
  cmd->height = img.range()[0].size();
  cmd->data.resize(static_cast<size_t>(cmd->width) * static_cast<size_t>(cmd->height));
  for (int y=0; y<cmd->height; ++y) {
    for (int x=0; x<cmd->width; ++x) {
      cmd->data[size_t(y)*size_t(cmd->width) + size_t(x)] = img[{y, x}];
    }
  }
  return cmd;
}

// Converter: PolyLine<float,2> -> shared_ptr<IRenderCommand> (AddPolylineOverlay2D)
 std::shared_ptr<IRenderCommand> makeCmdFromPolyLine2f(const Ravl2::PolyLine<float,2> &poly)
{
  auto cmd = std::make_shared<AddPolylineOverlay2D>(poly);
  // Channel + style hints configured by sink from URL
  return cmd;
}

// Converter: PointSet<float,3> -> shared_ptr<IRenderCommand> (SetPointCloud3D)
 std::shared_ptr<IRenderCommand> makeCmdFromPointSet3f(const Ravl2::PointSet<float,3> &ps)
{
  auto cmd = std::make_shared<SetPointCloud3D>(std::string{} /*channel set by sink from URL*/);
  cmd->positions.reserve(ps.size());
  for (const auto &p : ps) {
    if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2])) continue;
    cmd->positions.emplace_back(p[0], p[1], p[2]);
  }
  return cmd;
}

// Register type conversions when this TU is loaded.
[[maybe_unused]] bool g_registerConverters = [](){
  SPDLOG_DEBUG("Registering TypeConverter: Array<u8,2>/Array<f32,2>/Array<RGB8,2>/Array<i16,2>/Array<i32,2>/PolyLine2f/PointSet3f -> shared_ptr<IRenderCommand>");
  bool ok1 = registerConversion(makeCmdFromU8Array, 1.0f);
  bool ok2 = registerConversion(makeCmdFromF32Array, 0.95f);
  bool ok3 = registerConversion(makeCmdFromRGB8Array, 1.0f);
  bool ok4 = registerConversion(makeCmdFromI16Array, 1.0f);
  bool ok5 = registerConversion(makeCmdFromI32Array, 1.0f);
  bool ok6 = registerConversion(makeCmdFromPolyLine2f, 1.0f);
  bool ok7 = registerConversion(makeCmdFromPointSet3f, 1.0f);
  (void)ok1; (void)ok2; (void)ok3; (void)ok4; (void)ok5; (void)ok6; (void)ok7;
  return true;
}();

 inline uint32_t packColorRGBA255(uint8_t r,uint8_t g,uint8_t b,uint8_t a) noexcept {
  return (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(g) << 8) | uint32_t(r);
}

 inline bool parseColorRgba(std::string val, uint32_t &out) {
  // Accept #RRGGBBAA or r,g,b,a (floats 0..1)
  if (!val.empty() && val[0] == '#') {
    if (val.size() == 9) {
      auto hex = [&](char c)->int{ if (c>='0'&&c<='9') return c-'0'; if (c>='a'&&c<='f') return 10+(c-'a'); if (c>='A'&&c<='F') return 10+(c-'A'); return -1; };
      auto byteAt=[&](size_t i)->int{ int hi=hex(val[i]); int lo=hex(val[i+1]); if (hi<0||lo<0) return -1; return (hi<<4)|lo; };
      int r=byteAt(1), g=byteAt(3), b=byteAt(5), a=byteAt(7);
      if (r>=0&&g>=0&&b>=0&&a>=0) { out = packColorRGBA255(uint8_t(r),uint8_t(g),uint8_t(b),uint8_t(a)); return true; }
    }
    return false;
  }
  // r,g,b,a floats
  float rf=0,gf=0,bf=0,af=1;
  try {
    size_t p1 = val.find(','); if (p1==std::string::npos) return false;
    rf = std::stof(val.substr(0,p1));
    size_t p2 = val.find(',', p1+1); if (p2==std::string::npos) return false;
    gf = std::stof(val.substr(p1+1, p2-(p1+1)));
    size_t p3 = val.find(',', p2+1); if (p3==std::string::npos) return false;
    bf = std::stof(val.substr(p2+1, p3-(p2+1)));
    af = std::stof(val.substr(p3+1));
  } catch(...) { return false; }
  auto clamp01=[&](float v){ return v<0.f?0.f:(v>1.f?1.f:v); };
  uint8_t r = uint8_t(clamp01(rf)*255.f + 0.5f);
  uint8_t g = uint8_t(clamp01(gf)*255.f + 0.5f);
  uint8_t b = uint8_t(clamp01(bf)*255.f + 0.5f);
  uint8_t a = uint8_t(clamp01(af)*255.f + 0.5f);
  out = packColorRGBA255(r,g,b,a);
  return true;
}

struct OutputFormatDebugDisplayCmdSink : public Ravl2::OutputFormat {
  OutputFormatDebugDisplayCmdSink() : OutputFormat("DebugDisplayCmdSink", "", "display", 1100) {}

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

  static std::optional<std::string> getControlValue(const std::string &controls, std::string_view key) {
    auto pos = controls.find(key);
    if (pos == std::string::npos) return std::nullopt;
    auto val = controls.substr(pos + key.size());
    auto colon = val.find(':');
    if (colon != std::string::npos) val = val.substr(0, colon);
    return val;
  }
  static bool hasControl(const std::string &controls, std::string_view flag) {
    return controls.find(flag) != std::string::npos;
  }

  std::optional<StreamOutputPlan> probe(const ProbeOutputContext &ctx) override {
    auto parsed = parseDisplayUrl(ctx.m_url);
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
      auto parsed2 = parseDisplayUrl(url);
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

      // If this is a recognized command, adjust with URL context
      if (cmd) {
        // Check if it's any of the SetBaseImage2D template instantiations
        bool isImageCmd = false;
        if (auto *setImgU8 = dynamic_cast<SetBaseImage2D_U8*>(cmd.get())) {
          setImgU8->channel = parsed2->channel;
          isImageCmd = true;
        } else if (auto *setImgF32 = dynamic_cast<SetBaseImage2D_F32*>(cmd.get())) {
          setImgF32->channel = parsed2->channel;
          isImageCmd = true;
        } else if (auto *setImgRGB = dynamic_cast<SetBaseImage2D_RGB8*>(cmd.get())) {
          setImgRGB->channel = parsed2->channel;
          isImageCmd = true;
        } else if (auto *setImgI16 = dynamic_cast<SetBaseImage2D_I16*>(cmd.get())) {
          setImgI16->channel = parsed2->channel;
          isImageCmd = true;
        } else if (auto *setImgI32 = dynamic_cast<SetBaseImage2D_I32*>(cmd.get())) {
          setImgI32->channel = parsed2->channel;
          isImageCmd = true;
        }

        if (isImageCmd) {
          DebugDisplay::enqueue(cmd);
        } else if (auto *poly = dynamic_cast<AddPolylineOverlay2D*>(cmd.get())) {
          // Optional: clear overlays first
          if (!parsed2->controls.empty() && hasControl(parsed2->controls, ":ClearOverlays")) {
            auto clearOv = std::make_shared<ClearOverlays2D>(parsed2->channel);
            DebugDisplay::enqueue(clearOv);
          }
          // Set channel and style hints from controls
          poly->channel = parsed2->channel;
          // Mode
          if (auto mv = getControlValue(parsed2->controls, ":Mode=")) {
            if (*mv == "Replace") poly->mode = AddPolylineOverlay2D::Mode::Replace;
            else poly->mode = AddPolylineOverlay2D::Mode::Append;
          }
          // Closed
          if (auto cv = getControlValue(parsed2->controls, ":Closed=")) {
            poly->closed = (*cv == "true" || *cv == "1" || *cv == "True" || *cv == "TRUE");
          }
          // Width
          if (auto wv = getControlValue(parsed2->controls, ":Width=")) {
            try { poly->widthPx = std::max(0.1f, std::stof(*wv)); } catch(...) {}
          }
          // Color
          if (auto colv = getControlValue(parsed2->controls, ":Color=")) {
            uint32_t rgba{}; if (parseColorRgba(*colv, rgba)) poly->rgba = rgba; else {
              SPDLOG_WARN("DebugDisplay: invalid :Color spec '{}', keeping default", *colv);
            }
          }
          DebugDisplay::enqueue(cmd);
        } else if (auto *pc3d = dynamic_cast<SetPointCloud3D*>(cmd.get())) {
          // Default 3D routing: set channel from URL and enqueue
          pc3d->channel = parsed2->channel;
          DebugDisplay::enqueue(cmd);
        } else {
          // For other commands we just enqueue as-is.
          DebugDisplay::enqueue(cmd);
        }
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
  SPDLOG_DEBUG("Registering DebugDisplay 'display' command sink (shared_ptr<IRenderCommand>)");
  return outputFormatMap().add(std::make_shared<OutputFormatDebugDisplayCmdSink>());
}();

} // namespace
} // namespace Ravl2::DebugDisplay
