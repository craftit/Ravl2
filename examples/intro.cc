//
// Created by charles galambos on 24/07/2024.
//

#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>

// This file will be generated automatically when you run the CMake configuration step.
// It creates a namespace called `Ravl2`.
// You can modify the source template at `configured_files/config.hh.in`.
#include <internal_use_only/config.hh>

int main(int argc, char **argv)
{
  CLI::App app{"App description"};

  // Define options
  int p = 0;
  app.add_option("-p", p, "Parameter");

  bool show_version = false;
  app.add_flag("--version", show_version, "Show version information");

  CLI11_PARSE(app, argc, argv);

  if (show_version) {
    fmt::print("{}\n", Ravl2::cmake::project_version);
    return EXIT_SUCCESS;
  }

  SPDLOG_INFO("Parameter value: {}", p);
  return 0;
}