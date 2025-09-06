# Video Interface Specification for Non-Linear Markup Tool

## Core Concepts

1. **File Container**: Abstracts the source of video data (file, memory, network)
2. **Stream Iterator**: Provides access to individual streams within a container
3. **Frame Identifier**: Unique 64-bit identifier for each frame/audio chunk within a stream.
4. **Seek Operations**: Random access to time points in streams
5. **Thread Safety**: Support for concurrent access patterns

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

### FrameIdentifier (uint64_t)

- High bits: Stream identifier
- Middle bits: Timestamp/position identifier (PTS?)
- Low bits: Frame type and flags
- Deterministic and reproducible across sessions

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

6. **Extensibility**:
    - Plugin system for new container formats
    - Custom stream processors
    - Event system for monitoring stream state changes

7. **Allow Annotation Storage**:
    - Not provided by this interface but we will need to support it. 
    - Annotation indexed by frame identifier
    - Support for hierarchical annotation (stream, segment, frame)
    - Efficient lookup of annotations by time ranges

8. **Synchronization**:
    - Mechanisms to align frames from different streams
    - Handling of variable frame rates across streams
    - Compensation for clock drift between streams
    - Support variable frame rate streams

9. **Metadata Handling**:
    - Extraction and storage of metadata (e.g., EXIF, XMP)
    - Support for user-defined metadata fields
    - Efficient querying of metadata across streams

10. **Use Ravl2 containers for data presented for processing. **:
    - Use Ravl2 data containers for frames and audio chunks
    - Ensure compatibility with existing Ravl2 processing pipelines
    - Leverage Ravl2's memory management and threading capabilities
    - Use a wrapped class for the images which contains the identifier, time stamp as well as the image/audio/meta data.
    - The data should be presented in a decoded state that can be used for processing.

11. **Prototyping this interface**:
    - Unit tests and example code will be places in the 'tests' subdirectory.
    - Provide an 'in-memory' implementation for testing and use cases where memory is not an issue.
    - Use FFmpeg for file-based implementations
    - The first stage is to prototype the interface and get feedback.

This design allows for efficient non-linear access to video streams while supporting concurrent processing tasks like thumbnail generation, annotation, and playback.