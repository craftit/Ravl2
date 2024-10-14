
#include "Ravl2/OpenGL/GLWindow.hh"

#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>

int main(int argc,char **argv)
{
  using namespace Ravl2;

  CLI::App app{"OpenGL window example program"};

  bool verbose = false;
  app.add_flag("-v", verbose, "Verbose mode. ");

  CLI11_PARSE(app, argc, argv);

  if (verbose) {
    spdlog::set_level(spdlog::level::debug);
  }

  auto window = Ravl2::GLWindow(800, 600, "OpenGL Window");

  // Run the main loop
  window.runMainLoop();

  return 0;
}