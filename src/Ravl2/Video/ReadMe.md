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
- Thread-safe for concurrent access by multiple iterators
- Supports both file and memory-based sources
- Delegates actual decoding to FFmpeg or other backends

### StreamIterator

- Provides sequential access to frames/chunks from a specific stream
- Maintains position within a stream
- Only iterates over frames/chunks within a single stream
- Thread-independent from other iterators on the same container

## Important Design Considerations

1. **Separation of Concerns**:
    - Container handles resource management
    - Iterators handle navigation

2. **Thread Safety Model**:
    - Container is thread-safe for read operations
    - Each iterator maintains its own state
    - No shared mutable state between iterators

3. **Memory Management**:
    - Clear ownership model for decoded frames
    - Support for frame reference counting
    - Optional frame caching for frequent access patterns

4. **Error Handling**:
    - Robust error reporting
    - Recovery mechanisms for corrupted streams
    - Graceful degradation when frames are missing

5. **Performance Optimizations**:
    - Prefetching of frames during sequential access
    - Index-based seeking to reduce latency
    - Shared decoding context for multiple iterators when possible
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



---

This design allows for efficient non-linear access to video streams while supporting concurrent processing tasks like thumbnail generation, annotation, and playback.