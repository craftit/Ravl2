
#include "VideoTypes.hh"

namespace Ravl2::Video {

	std::string_view toString(StreamType type)
	{
		switch (type)
		{
			case StreamType::Video: return "Video";
			case StreamType::Audio: return "Audio";
			case StreamType::Subtitle: return "Subtitle";
			case StreamType::Data: return "Data";
			case StreamType::Unknown: return "Unknown";
			default: return "Unknown";
		}
	}

}
