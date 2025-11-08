#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/spdlog.h>
#include <cstring>

#include "Ravl2/Array.hh"
#include "Ravl2/IO/OutputFormat.hh"
#include "Ravl2/IO/StreamOutput.hh"
#include "Ravl2/Display/IRenderCommand.hh"
#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/DebugDisplay.hh"
#include "Ravl2/Display/Commands/SetBaseImage2D.hh"

namespace Ravl2::DebugDisplay {
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

// Local clear command to avoid using deprecated shim and keep Werror happy
struct ClearChannelCommand : public IRenderCommand {
  std::string channel;
  explicit ClearChannelCommand(std::string ch) : channel(std::move(ch)) {}
  void apply(ChannelRegistry &channels) override {
    channels.clearChannel(channel);
    SPDLOG_INFO("DebugDisplay: cleared channel '{}'", channel);
  }
};

// Common plan builder for a specific Array<T,2> type

template <typename T>
static std::optional<StreamOutputPlan> makePlanForArray2(const ProbeOutputContext &ctx) {
  if (!parseDebugUrl(ctx.m_url)) return std::nullopt; // only handle @debug URLs
  if (ctx.m_sourceType != typeid(Array<T,2>)) return std::nullopt;

  // StreamOutput that enqueues a SetBaseImage2D on write
  auto stream = std::make_shared<StreamOutputCall<Array<T,2>>>([](const Array<T,2> &img, std::streampos pos) -> std::streampos {
    (void)pos; (void)img;
    return 0;
  });

  // We cannot pass state into StreamOutputCall easily; instead, capture URL by wrapping in OutputFormat probe logic below.
  return StreamOutputPlan{.mStream = stream, .mConversion = {}, .mCost = 1.0f};
}

// Registration: OutputFormat that recognizes @debug URLs and returns a StreamOutput that enqueues commands.
struct OutputFormatDebugDisplay : public Ravl2::OutputFormat {
  OutputFormatDebugDisplay() : OutputFormat("DebugDisplay", "", "", 1000) {}

  std::optional<StreamOutputPlan> probe(const ProbeOutputContext &ctx) override {
    auto parsed = parseDebugUrl(ctx.m_url);
    if (!parsed) return std::nullopt;

    // We only handle exact types Array<uint8_t,2> and Array<float,2> in Phase 4
    if (ctx.m_sourceType == typeid(Array<uint8_t,2>)) {
      auto plan = makePlanForArray2<uint8_t>(ctx);
      if (!plan) return std::nullopt;
      // Replace stream with one that captures URL details
      plan->mStream = std::make_shared<StreamOutputCall<Array<uint8_t,2>>>([url=ctx.m_url](const Array<uint8_t,2> &img, std::streampos pos){
        (void)pos;
        auto p = parseDebugUrl(url);
        if (!p) return std::streampos(0);
        // Honor :Clear without deprecated shim
        if (!p->controls.empty() && p->controls.find(":Clear") != std::string::npos) {
          auto clearCmd = std::make_unique<ClearChannelCommand>(p->channel);
          DebugDisplay::ensureStarted({});
          DebugDisplay::enqueue(std::move(clearCmd));
        }
        auto cmd = std::make_unique<SetBaseImage2D>(p->channel);
        cmd->width = img.range()[1].size();
        cmd->height = img.range()[0].size();
        cmd->isFloat = false;
        cmd->u8.resize(static_cast<size_t>(cmd->width) * static_cast<size_t>(cmd->height));
        // Copy pixels honoring strides via element access
        for (int y=0; y<cmd->height; ++y) {
          for (int x=0; x<cmd->width; ++x) {
            cmd->u8[size_t(y)*size_t(cmd->width) + size_t(x)] = img[{y, x}];
          }
        }
        DebugDisplay::ensureStarted({});
        DebugDisplay::enqueue(std::move(cmd));
        return std::streampos(0);
      });
      return plan;
    }
    if (ctx.m_sourceType == typeid(Array<float,2>)) {
      auto plan = makePlanForArray2<float>(ctx);
      if (!plan) return std::nullopt;
      plan->mStream = std::make_shared<StreamOutputCall<Array<float,2>>>([url=ctx.m_url](const Array<float,2> &img, std::streampos pos){
        (void)pos;
        auto p = parseDebugUrl(url);
        if (!p) return std::streampos(0);
        if (!p->controls.empty() && p->controls.find(":Clear") != std::string::npos) {
          auto clearCmd = std::make_unique<ClearChannelCommand>(p->channel);
          DebugDisplay::ensureStarted({});
          DebugDisplay::enqueue(std::move(clearCmd));
        }
        auto cmd = std::make_unique<SetBaseImage2D>(p->channel);
        cmd->width = img.range()[1].size();
        cmd->height = img.range()[0].size();
        cmd->isFloat = true;
        cmd->f32.resize(static_cast<size_t>(cmd->width) * static_cast<size_t>(cmd->height));
        for (int y=0; y<cmd->height; ++y) {
          for (int x=0; x<cmd->width; ++x) {
            cmd->f32[size_t(y)*size_t(cmd->width) + size_t(x)] = img[{y, x}];
          }
        }
        DebugDisplay::ensureStarted({});
        DebugDisplay::enqueue(std::move(cmd));
        return std::streampos(0);
      });
      return plan;
    }

    return std::nullopt;
  }
};

[[maybe_unused]] bool g_registerDebugDisplay = [](){
  SPDLOG_INFO("Registering DebugDisplay @debug output handler (u8,f32 2D images)");
  return outputFormatMap().add(std::make_shared<OutputFormatDebugDisplay>());
}();

} // namespace
} // namespace Ravl2::DebugDisplay
