# RavlIO overview

RavlIO maps URLs and C++ types to registered format handlers. Loading and saving
use parallel APIs: `openInput()` / `openOutput()` select a plan, while
`openInputStream<T>()` / `openOutputStream<T>()` expose typed stream proxies.
The input and output registries use the same URL normalization, protocol
filtering, priority order, and default-handler fallback.

## URL parsing and handler selection

`parseUrl()` splits a URL into its protocol, filename, and extension. A URL
without `://` uses protocol `file`; for example, `image.PNG` becomes protocol
`file`, filename `image.PNG`, extension `png`. The extension is taken from the
filename path, lowercased, and stored without its leading dot. A URL such as
`camera://0` has protocol `camera` and no extension.

When opening a URL, RavlIO:

1. Looks up handlers registered for the normalized extension.
2. Tries matching protocol handlers in descending priority order; equal
   priorities retain registration order.
3. If no extension-specific handler returns a plan, tries handlers registered
   in the catch-all `""` extension bucket, still filtering by protocol.

`InputFormat` and `OutputFormat` registrations provide a comma-separated
extension list, a protocol, and a priority. `supportsExtension()` compares a
single extension case-insensitively against that list. Higher priority values
are tried first. Use a high-priority native handler to precede a generic
fallback handler.

## Plans and type conversion

A handler's `probe()` returns `std::nullopt` when it cannot handle the request,
or a plan describing its native stream and any conversion required by the
caller. `StreamInputPlan` contains a `StreamInputBase`, an optional conversion
from the handler's native type to the requested type, and an input score.
`StreamOutputPlan` contains a `StreamOutputBase`, an optional conversion from
the caller's type to the handler's native type, and `mCost`.

`typeConverterMap().find(to, from)` searches for a conversion chain from the
second argument to the first. For a reader, the common call is
`find(requestedType, nativeType)`; for a writer it is
`find(nativeType, sourceType)`. `ConversionChain::conversionLoss()` is a
preservation score in `[0, 1]`, with higher values indicating less information
loss. A plan's score is handler-provided; registries currently select handlers
by extension, protocol, and priority, not by comparing plan scores.

## Stream lifecycle

The typed input path is:

```cpp
auto input = Ravl2::openInputStream<MyImage>("input.png");
for(const auto &image : input) {
  // Process each image.
}
```

The output path writes successive objects and can explicitly finalize the
stream:

```cpp
auto output = Ravl2::openOutputStream<MyImage>("output.example");
output.put(image);
if(!output.close()) {
  // The format failed to finalize its output.
}
```

`close()` forwards through wrappers and reports whether finalization succeeded.
The `StreamOutputCall` destructor calls it as a fallback; explicit close is
preferred when the caller needs to observe errors. Ordinary streams may leave
the default close implementation in place. `put()` writes at the stream's
`endOffset()`; sequential encoders should not interpret that position as a
frame number.

## Registering an output format

The output format's probe selects a native type and returns a stream plus an
optional source-to-native conversion. The following illustrates the shape of a
handler; `writeValue()` and `closeWriter()` represent format-specific state and
operations:

```cpp
auto format = std::make_shared<Ravl2::OutputFormatCall>(
  "Example", "example", "file", 0,
  [](const Ravl2::ProbeOutputContext &ctx)
      -> std::optional<Ravl2::StreamOutputPlan> {
    std::function<std::any(std::any)> convert;
    float score = 1.0f;
    if(ctx.mSourceType != typeid(ExampleNative)) {
      auto conversion = Ravl2::typeConverterMap().find(
        typeid(ExampleNative), ctx.mSourceType);
      if(!conversion)
        return std::nullopt;
      const auto chain = *conversion;
      convert = [chain](std::any value) { return chain(value); };
      score = conversion->conversionLoss();
    }

    auto writer = makeWriter(ctx.mFilename);
    auto stream = std::make_shared<Ravl2::StreamOutputCall<ExampleNative>>(
      [writer](const ExampleNative &value, std::streampos pos) {
        writeValue(*writer, value);
        return pos;
      }, 0, std::numeric_limits<std::streampos>::max(),
      [writer] { return closeWriter(*writer); });
    return Ravl2::StreamOutputPlan {stream, std::move(convert), score};
  });
Ravl2::outputFormatMap().add(format);
```

For input formats, implement `InputFormat::probe()` similarly and return a
`StreamInputPlan`. Its stream callback takes `std::streampos&` and returns
`std::optional<T>`; return `std::nullopt` at end of input. Keep probing cheap,
and capture state needed during reading or writing in the returned stream.

## Thread safety and hints

The global input and output registries protect their maps while adding and
probing handlers. This does not make format handlers, returned streams, or
stream proxies thread-safe; treat each stream as single-threaded unless its
format explicitly documents otherwise. `ProbeInputContext` and
`ProbeOutputContext` are read-only during probing. `openInput()` and
`openOutput()` populate their `mVerbose` convenience fields from the JSON
`verbose` hint. Other `formatHint` keys are format-specific and should be
documented by their handler.

See `InputFormat.hh`, `OutputFormat.hh`, and `ImageIO/ImageIO_Overview.md` for
the format-specific probing and conversion details.