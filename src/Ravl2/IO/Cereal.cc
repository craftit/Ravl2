//
// Created by charles galambos on 24/08/2024.
//

#include "Ravl2/IO/Cereal.hh"
#include "Ravl2/IndexRangeSet.hh"
#include "Ravl2/Array.hh"

// This file will be generated automatically when you run the CMake configuration step.
// It creates a namespace called `Ravl2`.
// You can modify the source template at `configured_files/config.hh.in`.
#include "Ravl2/config.hh"

namespace Ravl2
{
  //! Enable cerial IO
  bool initCerealIO()
  {
    return true;

  }

  CerealArchiveHeader::CerealArchiveHeader(std::string theTypeName)
    : m_magic(m_magicNumber),
      fileFormatVersion(1),
      libVersion(Ravl2::cmake::project_version),
      gitHash(Ravl2::cmake::git_sha),
      typeName(std::move(theTypeName))
  {}


  //! Make sure these aren't instantiated in every translation unit.
  template class CerealSaveFormat<cereal::BinaryOutputArchive>;
  template class CerealLoadFormat<cereal::BinaryInputArchive>;
  template class CerealSaveFormat<cereal::JSONOutputArchive>;
  template class CerealLoadFormat<cereal::JSONInputArchive>;

  namespace
  {
    [[maybe_unused]] bool regFormat1 = registerCerealFormats<IndexRangeSet<2>>();
    [[maybe_unused]] bool regFormat2 = registerCerealFormats<Array<uint8_t,2>>();
    [[maybe_unused]] bool regFormat3= registerCerealFormats<Array<float,2>>();
    [[maybe_unused]] bool regFormat4 = registerCerealFormats<Array<int,2>>();
    [[maybe_unused]] bool regFormat5 = registerCerealFormats<Array<uint32_t,2>>();
  }// namespace
}// namespace Ravl2