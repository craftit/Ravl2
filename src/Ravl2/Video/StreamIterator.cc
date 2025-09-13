//
// Created on September 6, 2025
//

#include "Ravl2/Video/StreamIterator.hh"
#include "Ravl2/Video/MediaContainer.hh"
#include <stdexcept>

namespace Ravl2::Video {

	StreamType StreamIterator::streamType() const
	{
		if (!mContainer)
		{
			return StreamType::Unknown;
		}
		return mContainer->streamType(mStreamIndex);
	}

	int64_t StreamIterator::positionIndex() const
	{
		if (mCurrentFrame)
		{
			return mCurrentFrame->id();
		}
		return -1;
	}

	std::type_info const& StreamIterator::dataType() const
	{
		return typeid(void);
	}
} // namespace Ravl2::Video
