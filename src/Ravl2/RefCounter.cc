/*
 * RefCounter.cc
 *
 *  Created on: Dec 8, 2013
 *      Author: charlesgalambos
 */



#include "Ravl2/RefCounter.hh"
#include <assert.h>

namespace Ravl2
{

  //! Destructor.
  RefCounter::~RefCounter()
  {
    assert(m_referenceCount == 0);
  }

}
