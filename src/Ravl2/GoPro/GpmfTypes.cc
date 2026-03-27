//
// Created for RAVL2 GoPro metadata support
//

#include "Ravl2/GoPro/GpmfTypes.hh"
#include "Ravl2/Video/MediaContainer.hh"
#include "Ravl2/Types.hh"

namespace Ravl2::GoPro
{

  void typeInit()
  {}

  namespace
  {
    [[maybe_unused]] bool reg1 = Ravl2::Video::MediaContainer::registerDataType(typeid(GpsFix),Ravl2::Video::StreamType::Data);
    [[maybe_unused]] bool reg2 = Ravl2::Video::MediaContainer::registerDataType(typeid(GyroSamples),Ravl2::Video::StreamType::Data);
    [[maybe_unused]] bool reg3 = Ravl2::Video::MediaContainer::registerDataType(typeid(AccelSamples),Ravl2::Video::StreamType::Data);
    [[maybe_unused]] bool reg4 = Ravl2::Video::MediaContainer::registerDataType(typeid(GpsSamples),Ravl2::Video::StreamType::Data);

  }

}
