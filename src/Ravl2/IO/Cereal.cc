//
// Created by charles galambos on 24/08/2024.
//

#include "Ravl2/IO/Cereal.hh"
#include "Ravl2/IndexRangeSet.hh"

namespace Ravl2
{
  namespace {
    [[maybe_unused]] bool regFormat1 = saveFormatMap().add(std::make_unique<CerealSaveFormat<IndexRangeSet<2>,cereal::BinaryOutputArchive>>());
    [[maybe_unused]] bool regFormat2 = InputFormatMap().add(std::make_unique<CerealLoadFormat<IndexRangeSet<2>,cereal::BinaryInputArchive>>());
  }
}