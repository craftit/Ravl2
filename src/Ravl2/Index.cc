/*
 * Range.cc
 *
 *  Created on: Dec 8, 2013
 *      Author: charlesgalambos
 */

#include "Ravl2/Index.hh"

namespace Ravl2
{
  template class Index<1>;
  template class Index<2>;
  template class Index<3>;
  template class IndexRange<1>;
  template class IndexRange<2>;
  template class IndexRange<3>;
  template class IndexRangeIterator<1>;
  template class IndexRangeIterator<2>;
  template class IndexRangeIterator<3>;
}
