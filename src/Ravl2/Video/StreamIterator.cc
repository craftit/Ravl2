//
// Created on September 6, 2025
//

#include "Ravl2/Video/StreamIterator.hh"
#include <stdexcept>

namespace Ravl2::Video {

// This file implements the non-templated functionality of the StreamIterator interface
// Most of the functionality is defined inline in the header file

// Helper class implementations
// Currently all functionality is inline in the header

void StreamIterator::setPrefetchEnabled(bool enable)
{
  (void)enable;
}

void StreamIterator::setPrefetchSize(int size)
{
  (void)size;
}

bool StreamIterator::isPrefetchEnabled() const
{
  return false;
}

int StreamIterator::prefetchSize() const
{
  return 0;
}

} // namespace Ravl2::Video
