//
// Created by charles galambos on 12/06/2026.
//
//! Harris-Affine feature detection example: detects affine-covariant elliptical
//! regions and draws them on the input image.

#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>

#include "Ravl2/config.hh"
#include "Ravl2/Image/HarrisAffineDetector.hh"
#include "Ravl2/Image/DrawEllipse.hh"
#include "Ravl2/Image/DrawCross.hh"
#include "Ravl2/OpenCV/ImageIO.hh"
#include "Ravl2/IO/Load.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/Resource.hh"

int main(int argc, char **argv)
{
  Ravl2::initOpenCVImageIO();

  CLI::App app {"Harris-Affine feature detection example program"};

  Ravl2::HarrisAffineConfig config;
  float regionScale = 3.0f;
  bool verbose = false;

  app.add_option("-t,--threshold", config.threshold, "Harris threshold as a fraction of the per-level maximum.");
  app.add_option("-k", config.k, "Harris k parameter.");
  app.add_option("--sigma0", config.sigma0, "First integration scale.");
  app.add_option("--num-scales", config.numScales, "Number of scale levels.");
  app.add_option("--max-features", config.maxFeatures, "Maximum number of features to adapt.");
  app.add_option("--region-scale", regionScale, "Drawn region radius in characteristic sigmas.");
  app.add_flag("-v", verbose, "Verbose mode.");

  std::string inf = Ravl2::findFileResource("data", "lena.jpg", verbose);
  std::string outf = "display://HarrisAffine";

  app.add_option("-i", inf, "Input image.");
  app.add_option("-o", outf, "Output image.");

  bool show_version = false;
  app.add_flag("--version", show_version, "Show version information");

  CLI11_PARSE(app, argc, argv);

  if(show_version) {
    fmt::print("{}\n", Ravl2::cmake::project_version);
    return EXIT_SUCCESS;
  }

  Ravl2::Array<uint8_t, 2> img;
  if(!Ravl2::ioLoad(img, inf)) {
    SPDLOG_ERROR("Failed to load image '{}'", inf);
    return 1;
  }

  Ravl2::HarrisAffineDetector detector(config);
  std::vector<Ravl2::AffineFeature> features = detector.apply(img);
  SPDLOG_INFO("Found {} affine features", features.size());

  const uint8_t value = 255;
  for(const auto &feature : features) {
    if(verbose) {
      SPDLOG_INFO("pos=({:.1f},{:.1f}) scale={:.2f} response={:.3g} shape=[{:.2f} {:.2f}; {:.2f} {:.2f}]",
                  feature.position[0], feature.position[1], feature.scale, feature.response,
                  feature.shape(0, 0), feature.shape(0, 1), feature.shape(1, 0), feature.shape(1, 1));
    }
    Ravl2::DrawCross(img, value, Ravl2::toIndex<2>(feature.position), 3);
    Ravl2::DrawEllipse(img, value, feature.ellipse(regionScale));
  }

  if(!Ravl2::ioSave(outf, img)) {
    SPDLOG_ERROR("Failed to save image");
    return 1;
  }

  return 0;
}
