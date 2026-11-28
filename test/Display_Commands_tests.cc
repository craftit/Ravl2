//
// Display: display:// URL control parsing, and commands applied to a channel registry (headless).
//

#include <catch2/catch_test_macros.hpp>

#include "Ravl2/Display/AddPolylineOverlay2D.hh"
#include "Ravl2/Display/Channel.hh"
#include "Ravl2/Display/CompositeNode.hh"
#include "Ravl2/Display/Image2DNode.hh"
#include "Ravl2/Display/SetBaseImage2D.hh"
#include "Ravl2/Display/SetNormalization2D.hh"
#include "Ravl2/Display/UrlControls.hh"

namespace Ravl2::DebugDisplay
{
  TEST_CASE("Display URL flags match whole tokens")
  {
    using namespace UrlControls;
    CHECK(hasFlag(":Clear", ":Clear"));
    CHECK(hasFlag(":Norm=Auto:clear", ":Clear"));
    // ":Clear" used to match these and wipe the whole channel.
    CHECK_FALSE(hasFlag(":ClearOverlays", ":Clear"));
    CHECK_FALSE(hasFlag(":ClearPlot:Mode=Replace", ":Clear"));
    CHECK(hasFlag(":ClearPlot:Mode=Replace", ":ClearPlot"));
  }

  TEST_CASE("Display URL values are matched by key, case-insensitively")
  {
    using namespace UrlControls;
    CHECK(value(":mode=replace:Series=a", ":Mode=") == std::optional<std::string>("replace"));
    CHECK(iequals(*value(":Mode=ringbuffer", "Mode"), "RingBuffer"));
    CHECK_FALSE(value(":Modes=x", ":Mode=").has_value());
  }

  TEST_CASE("Display URL normalization controls")
  {
    using namespace UrlControls;
    auto pct = parseNormalization(":Norm=Pct:5,95");
    REQUIRE(pct.has_value());
    CHECK(pct->policy == NormalizationPolicy::Percentile);
    CHECK(pct->lowPct == 5.0f);// used to fall back to the default 1
    CHECK(pct->highPct == 95.0f);

    auto pct2 = parseNormalization(":Clear:Normalize=Percentile=2,98:Mode=Append");
    REQUIRE(pct2.has_value());
    CHECK(pct2->lowPct == 2.0f);

    auto fixed = parseNormalization(":norm=fixed=0,255");
    REQUIRE(fixed.has_value());
    CHECK(fixed->policy == NormalizationPolicy::Fixed);
    CHECK(fixed->maxVal == 255.0f);

    auto autoNorm = parseNormalization(":Norm=Auto");
    REQUIRE(autoNorm.has_value());
    CHECK(autoNorm->policy == NormalizationPolicy::Auto);
    CHECK_FALSE(parseNormalization(":Mode=Replace").has_value());
  }

  TEST_CASE("Image updates and normalization reach an image under overlays")
  {
    ChannelRegistry channels;
    auto setImage = [&](float value) {
      SetBaseImage2D<float> cmd("img");
      cmd.width = 2;
      cmd.height = 2;
      cmd.data.assign(4, value);
      cmd.apply(channels);
    };
    setImage(1.0f);

    AddPolylineOverlay2D overlay(PolyLine<float, 2>({toPoint<float>(0, 0), toPoint<float>(1, 1)}));
    overlay.channel = "img";
    overlay.apply(channels);

    NormalizationSettings fixed {};
    fixed.policy = NormalizationPolicy::Fixed;
    fixed.minVal = 0.0f;
    fixed.maxVal = 10.0f;
    SetNormalization2D norm("img", fixed);
    norm.apply(channels);

    // A new image must not drop the overlay, and must keep the channel's normalization.
    setImage(2.0f);

    auto &ch = channels.getOrCreateChannel("img");
    auto *composite = dynamic_cast<CompositeNode *>(ch.sceneContent.get());
    REQUIRE(composite != nullptr);
    CHECK(composite->childCount() == 2);
    auto *image = dynamic_cast<Image2DNode<float> *>(baseContentSlot(ch.sceneContent).get());
    REQUIRE(image != nullptr);
    CHECK(image->getNormalization().policy == NormalizationPolicy::Fixed);
    CHECK(image->getData()[0] == 2.0f);
    CHECK(image->sample(0, 0).second == 0.2f);// 2 on a fixed [0,10] scale
  }
}// namespace Ravl2::DebugDisplay
