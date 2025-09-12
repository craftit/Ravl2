//
// Created by charles galambos on 12/09/2025.
//

#include "Ravl2/Catch2checks.hh"
#include "Ravl2/Resource.hh"
#include "Ravl2/IO/InputSequence.hh"
#include "Ravl2/Pixel/Pixel.hh"
#include "Ravl2/Video/VideoIO.hh"

namespace Ravl2::Video
{
  TEST_CASE("VideoStreamIO", "[IO]")
  {
    initIO(); // make sure video io is linked.
    std::string fn = "sample-5s.mp4";
    addResourcePath("data", RAVL_SOURCE_DIR "/data");
    fn = Ravl2::findFileResource("data", fn, true);

    using ImageT = Ravl2::Array<Ravl2::PixelRGB8,2>;
    StreamInputProxy<ImageT> inputStream = Ravl2::openInputStream<ImageT>(fn,defaultLoadFormatHint(true));
    CHECK(inputStream.valid());
    auto image = inputStream.get();
    CHECK(!image.empty());
  }

}
