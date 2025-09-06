
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


	std::string_view toString(VideoErrorCode error)
	{
		switch (error)
		{
			case VideoErrorCode::Success: return "Success";
			case VideoErrorCode::NeedMoreData: return "NeedMoreData";
			case VideoErrorCode::EndOfStream: return "EndOfStream";
			case VideoErrorCode::InvalidOperation: return "InvalidOperation";
			case VideoErrorCode::NotImplemented: return "NotImplemented";
			case VideoErrorCode::ResourceUnavailable: return "ResourceUnavailable";
			case VideoErrorCode::DecodingError: return "DecodingError";
			case VideoErrorCode::SeekFailed: return "SeekFailed";
			case VideoErrorCode::CorruptedData: return "CorruptedData";
			case VideoErrorCode::UnsupportedFormat: return "UnsupportedFormat";
			case VideoErrorCode::ResourceAllocationError: return "ResourceAllocationError";
			default:
				break;
		}
		return "UnknownError";
	}
}
