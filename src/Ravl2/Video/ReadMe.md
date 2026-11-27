# Video Interface Specification for Non-Linear Markup Tool

## Core Concepts

1. **File Container**: Abstracts the source of video data (file, memory, network)
2. **Stream Iterator**: Provides access to individual streams within a container
3. **Seek Operations**: Random access to time points in streams
4. **Thread Safety**: Support for concurrent access patterns

## Key Components

### MediaContainer

- Represents a video file/source containing multiple streams
- Maintains metadata about all streams (codec, resolution, duration)
- Container metadata queries are thread-safe (shared_mutex protected)
- Supports both file-based (FfmpegMediaContainer) and in-memory (MemoryMediaContainer) sources
- Delegates actual decoding to FFmpeg or other backends

### StreamIterator

- Provides sequential access to frames/chunks in presentation timestamp order
- Maintains position within the stream(s)
- Can iterate over a single stream or multiple streams simultaneously (FfmpegMultiStreamIterator delivers frames from all selected streams in temporal order)
- Each iterator maintains its own packet queue and frame state

## Important Design Considerations

1. **Separation of Concerns**:
    - Container handles resource management
    - Iterators handle navigation

2. **Thread Safety Model**:
    - Container metadata queries (stream count, properties, duration) are thread-safe
    - Each iterator maintains its own packet queue and frame state
    - **FfmpegMediaContainer limitation**: Iterators share the underlying FFmpeg format context and codec contexts with the container. Multiple iterators on the same FfmpegMediaContainer must NOT be used concurrently from different threads. For parallel processing, open a separate FfmpegMediaContainer per thread
    - MemoryMediaContainer iterators are independent and safe for concurrent use

3. **Memory Management**:
    - Clear ownership model for decoded frames
    - Support for frame reference counting
    - Optional frame caching for frequent access patterns

4. **Error Handling**:
    - Robust error reporting
    - Recovery mechanisms for corrupted streams
    - Graceful degradation when frames are missing

5. **Performance Optimizations**:
    - Prefetching of frames via priority queue for B-frame reordering and temporal ordering
    - Keyframe index built lazily on first seek for byte-position seeking
    - Zero-copy video frame delivery using shared_ptr aliasing to FFmpeg frame buffers
    - Audio frames are copied out of FFmpeg buffers (allowing immediate buffer reuse)
    - Each frame has a unique identifier (integer) for persistent referencing within a stream
      - Used for storing annotations in other parts of the software

6. **Synchronization**:
    - Mechanisms to align frames from different streams
    - Handling of variable frame rates across streams
    - Compensation for clock drift between streams
    - Support for variable frame rate streams

7. **Metadata Handling**:
    - Extraction and storage of file level metadata (e.g., EXIF, XMP)
    - Support for metadata streams associated with the video.
    - Efficient querying of metadata across streams

8. **Use Ravl2 containers for data presented for processing**:
    - Use Ravl2 data containers for frames and audio chunks
    - Ensure compatibility with existing Ravl2 processing pipelines
    - Leverage Ravl2's memory management and threading capabilities
    - Use a wrapped class for images containing the identifier, timestamp, and image/audio/meta data
    - Data should be presented in a decoded state for processing
    - See Ravl2/Pixel/.. for pixel types. Use Array<*Pixel,2> for images
    - For audio, use Array<float> or Array<short> depending on the source format. For multiple channels, use Array<float,2>
    - Follow Ravl2 code style and idioms
    - For time use std::chrono classes when dealing with time-based operations where sensible. Ideally keep the concepts of a time and a duration separate.

9. **Prototyping this interface**:
    - Unit tests and example code will be placed in the 'tests' subdirectory
    - Provide an 'in-memory' implementation for testing and use cases where memory is not an issue
    - Use FFmpeg for file-based implementations
    - The first stage is to prototype the interface and get feedback

## Writing compressed media

When FFmpeg support is enabled, RavlIO can write video files through
`openOutputStream<T>()`. The default video encoder is H.264 (preferring
`libx264`), with CRF 23 and a medium preset; MPEG-4 is used when H.264 is not
available or the selected container defaults to another codec. The first
image determines the output dimensions. Supported direct inputs are
`Array<PixelRGB8, 2>`, `Array<PixelBGR8, 2>`, and `YUV420Image<uint8_t>`; other
image types are accepted when the type-conversion registry can convert them to
RGB8.

```cpp
auto output = Ravl2::openOutputStream<Ravl2::Array<Ravl2::PixelRGB8, 2>>(
  "output.mp4", {{"fps", 30}, {"crf", 23}, {"preset", "medium"}});
output.put(image);
if(!output.close()) {
  // The encoder or container could not be finalized.
}
```

Timestamped video and audio can be written as `std::shared_ptr<Frame>` objects.
Use `FrameData<ImageT>` with `StreamType::Video` for video, and
`FrameData<AudioChunk<float>>` or `FrameData<AudioChunk<int16_t>>` with
`StreamType::Audio` for audio. Audio arrays are organized as samples by
channels. The first timestamp encountered establishes a shared media time
origin; video and audio timestamps are preserved relative to it. Plain image
streams use the configured fixed frame rate.

The JSON format hint accepts `fps`, `codec`, `crf`, `bitrate`, `preset`, `gop`,
`pixelFormat`, and `hwAccel` (`"none"` by default, or `"auto"` to try FFmpeg's
VideoToolbox, NVENC, QSV, or AMF encoders before falling back to software). Set
`audio` to `true` or to an object containing `codec`, `sampleRate`, `channels`,
and `bitrate` to declare an audio stream. AAC is the default audio codec.
Calling `close()` flushes delayed packets and writes the container trailer;
destruction also closes the stream as a fallback.



---

This design allows for efficient non-linear access to video streams while supporting concurrent processing tasks like thumbnail generation, annotation, and playback.