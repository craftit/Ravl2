#include "Ravl2/Display/Backends/ImguiBgfxBridge.hh"

#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
// Use the imgui helper provided with bgfx examples
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include <imgui.h>
#include "Ravl2/Display/bgfx_imgui/ImGUI/imgui.hh"
#pragma GCC diagnostic pop
#endif

namespace Ravl2::DebugDisplay {

bool ImguiBgfxBridge::init(float fontSize) noexcept {
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
  if (mInitialized) return true;
  imguiCreate(fontSize, nullptr);
  mInitialized = true;
  SPDLOG_INFO("ImguiBgfxBridge: initialized (fontSize={})", fontSize);
  return true;
#else
  (void)fontSize;
  return false;
#endif
}

void ImguiBgfxBridge::shutdown() noexcept {
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
  if (!mInitialized) return;
  imguiDestroy();
  mInitialized = false;
  SPDLOG_INFO("ImguiBgfxBridge: shutdown");
#endif
}

void ImguiBgfxBridge::beginFrame(int mouseX,
                                 int mouseY,
                                 uint8_t mouseButtons,
                                 int32_t scroll,
                                 uint16_t fbWidth,
                                 uint16_t fbHeight) noexcept {
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
  if (!mInitialized) return;
  imguiBeginFrame(mouseX, mouseY, mouseButtons, scroll, fbWidth, fbHeight);
#else
  (void)mouseX; (void)mouseY; (void)mouseButtons; (void)scroll; (void)fbWidth; (void)fbHeight;
#endif
}

void ImguiBgfxBridge::endFrame() noexcept {
#if defined(RAVL2_WITH_IMGUI) && defined(RAVL2_WITH_BGFX)
  if (!mInitialized) return;
  imguiEndFrame();
#endif
}

} // namespace Ravl2::DebugDisplay
