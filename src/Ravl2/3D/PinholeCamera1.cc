// This file is part of RAVL, Recognition And Vision Library
// Copyright (C) 2001-12, University of Surrey
// This code may be redistributed under the terms of the GNU Lesser
// General Public License (LGPL). See the lgpl.licence file for details or
// see http://www.gnu.org/copyleft/lesser.html
// file-header-ends-here
#include "Ravl2/3D/PinholeCamera1.hh"

namespace Ravl2
{

  template class PinholeCamera1<float>;
  
  namespace
  {
    [[maybe_unused]] static bool g_register1 = Ravl2::defaultConfigFactory().registerDirectType<PinholeCamera1<float>>("PinholeCamera1<float>");
  }
  
};
