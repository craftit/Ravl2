//
// Created by charles galambos on 22/10/2024.
//

#include "Ravl2/OpenGL/ImGui.hh"

#define USE_OPENGL3 1

#include "backends/imgui_impl_glfw.h"

#if USE_OPENGL3

#include "backends/imgui_impl_opengl3.h"

#else
#include "backends/imgui_impl_opengl2.h"
#endif

#include <stdio.h>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif

#include <GLFW/glfw3.h> // Will drag system OpenGL headers

namespace Ravl2
{

  ImGUI::ImGUI(std::shared_ptr<Ravl2::GLWindow> window)
    : mWindow(std::move(window))
  {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(mWindow->getGLFWwindow(), true);
#if USE_OPENGL3
    ImGui_ImplOpenGL3_Init(window->glslVersion());
#else
    ImGui_ImplOpenGL2_Init();
#endif

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

  }

  ImGUI::~ImGUI()
  {
    shutdown();
  }

  void
  ImGUI::newFrame()
  {
    if(mWindow == nullptr)
      return;
    // Start the Dear ImGui frame
#if USE_OPENGL3
    ImGui_ImplOpenGL3_NewFrame();
#else
    ImGui_ImplOpenGL2_NewFrame();
#endif
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

  }

  void
  ImGUI::render()
  {
    if(mWindow == nullptr)
      return;

    // Rendering
    ImGui::Render();
#if USE_OPENGL3
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#endif

  }

  void
  ImGUI::shutdown()
  {
    if(mWindow == nullptr)
      return;
    // Cleanup
#if USE_OPENGL3
    ImGui_ImplOpenGL3_Shutdown();
#else
    ImGui_ImplOpenGL2_Shutdown();
#endif
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    mWindow = nullptr;
  }

} // Ravl2