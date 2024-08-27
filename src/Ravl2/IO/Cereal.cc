//
// Created by charles galambos on 24/08/2024.
//

#include "Ravl2/IO/Cereal.hh"
#include "Ravl2/IndexRangeSet.hh"

namespace Ravl2
{
  //! Make sure these arn't instantiated in every translation unit.
  template class CerealSaveFormat<cereal::BinaryOutputArchive>;
  template class CerealLoadFormat<cereal::BinaryInputArchive>;
  template class CerealSaveFormat<cereal::JSONOutputArchive>;
  template class CerealLoadFormat<cereal::JSONInputArchive>;

  namespace
  {
    [[maybe_unused]] bool regFormat = registerCerealFormats<IndexRangeSet<2>>();


  }// namespace
}// namespace Ravl2