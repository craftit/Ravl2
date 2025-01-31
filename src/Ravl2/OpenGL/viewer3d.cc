
#include "DCube3D.hh"

#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>

#include "Ravl2/OpenGL/GLWindow.hh"
#include "Ravl2/OpenGL/Canvas3D.hh"
#include "Ravl2/OpenGL/View3D.hh"

#define USE_OPENGL3 1
#define USE_IMGUI 1

#if USE_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#if USE_OPENGL3
#include "backends/imgui_impl_opengl3.h"
#else
#include "backends/imgui_impl_opengl2.h"
#endif
#endif

static void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int argc,char **argv)
{

  CLI::App app{"OpenGL window example program"};

  bool verbose = false;
  app.add_flag("-v", verbose, "Verbose mode. ");

  CLI11_PARSE(app, argc, argv);

  if(verbose) {
    spdlog::set_level(spdlog::level::debug);
  }

  auto window = std::make_shared<Ravl2::GLWindow>(800, 600, "OpenGL Window");

  //! Create a 3D canvas
  //Ravl2::Canvas3D canvas(window);

  window->makeCurrent();

  Ravl2::CallbackSet callbacks;
#if !USE_IMGUI
  int display_w = 0;
  int display_h = 0;
  glfwGetFramebufferSize(window->getGLFWwindow(), &display_w, &display_h);
  Ravl2::View3D view(display_w, display_h, false, false);
  view.setContext(window);
  view.setup(*window);
  window->makeCurrent();
  view.GUIInitGL();
  auto cube = std::make_shared<Ravl2::DCube3D>();
  view.add(cube);
  //view.GUIAdjustView();

  callbacks += window->addFrameRender([&view]() {
    view.GUIRefresh();
  });

#else

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
  ImGui_ImplGlfw_InitForOpenGL(window->getGLFWwindow(), true);
#if USE_OPENGL3
  ImGui_ImplOpenGL3_Init(window->glslVersion());
#else
  ImGui_ImplOpenGL2_Init();
#endif

  // Our state
  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // Main loop
  while (!glfwWindowShouldClose(window->getGLFWwindow())) {
    //SPDLOG_INFO("Main loop");
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();

    // Start the Dear ImGui frame
#if USE_OPENGL3
    ImGui_ImplOpenGL3_NewFrame();
#else
    ImGui_ImplOpenGL2_NewFrame();
#endif
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    //if(show_demo_window)ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

      ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
      ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_window);

      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
      ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

      if(ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
	counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
      ImGui::End();
    }

    // 3. Show another simple window.
    if(show_another_window) {
      ImGui::Begin("Another Window",
		   &show_another_window
		  );   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if(ImGui::Button("Close Me"))
	show_another_window = false;
      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window->getGLFWwindow(), &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x * clear_color.w,
		 clear_color.y * clear_color.w,
		 clear_color.z * clear_color.w,
		 clear_color.w
		);
    glClear(GL_COLOR_BUFFER_BIT);

    // If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
    // you may need to backup/reset/restore other state, e.g. for current shader using the commented lines below.
//    GLint last_program;
//    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
//    glUseProgram(0);
#if USE_OPENGL3
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#endif
//    glUseProgram(last_program);

    window->swapBuffers();
  }

#endif
  // Run the main loop
  window->runMainLoop();

  return 0;
}
