// This file is part of RAVL, Recognition And Vision Library 
// Copyright (C) 2002, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
/////////////////////////////////////////////////////////////////////
//! author="Charles Galambos"

#include "Ravl2/IO/Cereal.hh"
#include "Ravl2/3D/TriMesh.hh"

namespace Ravl2
{

  template class TriMesh<float>;

  namespace {
    [[maybe_unused]] bool g_reg = registerCerealFormats<TriMesh<float> >();
  }
}
