# Capture devices

How to read live cameras through RavlIO, what you get, and where capture is heading.

```cpp
auto camera = Ravl2::openInputStream<Ravl2::Array<Ravl2::PixelRGB8, 2>>(
    "camera:///dev/video8?size=1280x720&fps=60&format=mjpeg&latest=1");
while(true) {
  auto image = camera.get();   // throws or ends the stream when the device stops
  ...
}
```

## URL forms

`camera://<device>[?<option>=<value>&...]`

| Device part | Meaning |
|---|---|
| `/dev/video8` | A V4L2 device path (Linux). |
| `8` | An index. On Linux it means `/dev/video8`. On macOS it is the AVFoundation device index. |
| *(empty)* | The platform default (`/dev/video0` on Linux, device 0 on macOS). |

| Option | Example | Meaning |
|---|---|---|
| `size` | `size=1280x720` | Frame size, width x height. |
| `fps` | `fps=60` | Frame rate. |
| `format` | `format=mjpeg` | Capture format, as FFmpeg/V4L2 names it: `mjpeg`, `yuyv422`, `nv12`, `h264`, and so on. `yuyv`, `yuy2`, `mjpg` and `jpeg` are accepted too. |
| `latest` | `latest=1` | Deliver only the newest frame (see [Buffering](#buffering)). |

The device part never includes the query, so `/dev/video8?size=...` opens `/dev/video8`.

An option that is left out uses the device's default. That is usually its first mode, often 640x480.

### Errors

Capture fails with `std::invalid_argument` and a message, instead of quietly doing something else, when:
- an option is unknown or malformed (`Unknown capture URL option 'colour'`);
- the device doesn't offer a requested setting. The message names the setting and lists every mode the device
  has, for example `Capture device '/dev/video8' doesn't support size=800x600. It offers:` followed by lines such
  as `mjpeg 1280x720 @ 60, 30 fps`;
- the device opened in a different mode from the one requested. FFmpeg's V4L2 input lets the driver substitute a
  size it can do, and only logs a warning, so the opened stream is checked against the request.

A device that can't be opened at all, when no options were given, isn't an error at this stage. Other handlers
get a chance, and `openInputStream` returns a stream that isn't `valid()`.

### Which handler opens it

1. **FFmpegCapture** (priority 0) is tried first, for every form. It uses V4L2 on Linux, AVFoundation on macOS
   and DirectShow on Windows. It is the only handler that understands options.
2. **OpenCV** (priority -1) is the fallback, for a bare index only (`camera://1`, no options), when FFmpeg can't
   open the device. It handles grey, BGR and BGRA frames.

Video files go through the same interface (`openInputStream<...>("clip.mp4")`). `latest` doesn't apply to
files.

## What you get

**Frame types.** Any image type there's a conversion to. The device's frames are decoded (MJPEG) or converted
(YUYV, NV12, ...), so `Array<PixelRGB8, 2>` works whatever the camera's format. YUV is full range, RAVL2's
convention; video is usually limited range and is expanded on the way in.

**Timestamps.** The RavlIO stream yields bare images, without timestamps. To get capture times, use the Video
API directly:

```cpp
auto container = Ravl2::Video::FfmpegMediaContainer::openDevice(params).value();
auto frames = container->createIterator(0).value();
frames->currentFrame()->timestamp();   // capture time, microseconds
```

For V4L2 that is the kernel's buffer timestamp, which FFmpeg converts to wall-clock time in its default
timestamp mode. It is taken when the frame was captured, not when your code received it. Carrying timestamps
through RavlIO (a timed-frame target type) is open item GAP-4 in `ReviewFindings.md`.

## Buffering

**Default.** Reads are in order and block until the next frame arrives. If you read more slowly than the camera
delivers, frames queue up. FFmpeg's V4L2 input asks the driver for up to 256 buffers, so latency can grow to
seconds before the driver starts dropping frames.

**`latest=1`.** A background thread reads continuously and keeps only the newest frame. Each `get()` waits for a
frame newer than the last one returned, and the frames in between are dropped. This is what a live recogniser
wants: low latency rather than every frame. The same behaviour is available through the Video API as
`Ravl2::Video::LatestFrameReader`, which also counts the frames it dropped.

## Design notes for future work

### A native V4L2 backend

FFmpeg's V4L2 input covers sizes, rates, formats and timestamps, which is all the requirements so far. A native
backend (V4L2 ioctls with mmap or DMABUF buffers, registered as another `camera` handler) would be worth writing
for any of these:
- **Camera controls**: exposure, gain, focus, white balance and power-line frequency (`VIDIOC_S_CTRL`). These
  matter for vision work, and FFmpeg doesn't expose them.
- **Buffer control**: a small, fixed buffer count for low latency without a reader thread, and a count of the
  frames the driver dropped.
- **Monotonic timestamps**: the kernel's `CLOCK_MONOTONIC` capture time, unconverted. This is needed for syncing
  with other sensors on the same clock.
- **Zero-copy**: YUYV or NV12 frames mapped straight into RAVL2 arrays.
- **Several cameras** opened and triggered together.

`CaptureDevice.hh` (URL parsing, `listCaptureModes()`, `checkCaptureRequest()`) doesn't depend on FFmpeg so that
such a backend can share it. URLs would stay the same, and a backend could be chosen with an option such as
`backend=v4l2`.

### Audio capture

Audio isn't implemented, but the capture design should leave room for it:
- **URLs.** The same scheme would work for audio, for example `audio://hw:1,0?rate=48000&channels=2&format=s16`
  for ALSA, or `audio://default` for PulseAudio or PipeWire. FFmpeg's `alsa` and `pulse` inputs produce streams
  that the existing iterator already decodes to `AudioChunk<T>`.
- **Never drop audio.** `latest` makes no sense for audio. Audio needs a queue sized to the processing
  deadline, and a count of overruns when it fills.
- **One clock.** Capture timestamps for video (V4L2) and audio (ALSA) must share a clock, or carry their clock
  domain, so that streams can be aligned. Raw monotonic timestamps are another reason for a native backend.
- **Devices with both** (the StreamCam has a microphone) appear to the OS as separate video and audio devices.
  The plan is one container per device, aligned by timestamp. A combined `MediaContainer` holding both streams
  could come later.
- **The interface.** A RavlIO stream of images can't carry audio. Audio-visual capture belongs to the Video API
  (`MediaContainer` and `StreamIterator`), which already supports several streams of different types.
