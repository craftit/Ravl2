# Ravl2 Debug Display - Time Series Plots User Guide

This guide explains how to use the time series plotting feature in Ravl2's Debug Display system.

## Table of Contents
- [Quick Start](#quick-start)
- [Basic Usage](#basic-usage)
- [Update Modes](#update-modes)
- [URL Controls Reference](#url-controls-reference)
- [Advanced Examples](#advanced-examples)
- [API Reference](#api-reference)
- [Tips and Best Practices](#tips-and-best-practices)

---

## Quick Start

The simplest way to plot data:

```cpp
#include "Ravl2/Array.hh"
#include "Ravl2/IO/Save.hh"

std::vector<float> data = {1.0, 2.5, 3.7, 2.1, 1.5};
ioSave("display://myplot:series=data", data);
```

This creates a plot in channel "myplot" with a series named "data". The Plots panel will appear automatically in the Debug Display window.

---

## Basic Usage

### Plotting a Vector

```cpp
std::vector<float> temperatures = {20.5, 21.2, 22.1, 21.8, 20.9};
ioSave("display://sensor:series=temperature", temperatures);
```

### Plotting a Ravl2 Array

```cpp
Array<float, 1> samples = /* ... */;
ioSave("display://scope:series=signal", samples);
```

### Multiple Series in One Plot

```cpp
std::vector<float> sine_data = {0.0, 0.5, 1.0, 0.5, 0.0};
std::vector<float> cosine_data = {1.0, 0.5, 0.0, -0.5, -1.0};

ioSave("display://trig:series=sin", sine_data);
ioSave("display://trig:series=cos", cosine_data);
```

Both series will appear in the same plot with different colors in the legend.

### Multiple Channels (Separate Plots)

```cpp
ioSave("display://sensor1:series=voltage", sensor1_data);
ioSave("display://sensor2:series=voltage", sensor2_data);
```

Each channel gets its own plot in the Plots panel.

---

## Update Modes

The Debug Display supports three update modes for series data:

### Mode: Append (Default)

Adds new data points to the end of the existing series. Perfect for streaming/real-time data.

```cpp
// Continuously append data
for(int i = 0; i < 1000; ++i) {
    float value = std::sin(i * 0.1f);
    std::vector<float> sample = {value};
    ioSave("display://scope:series=signal:mode=append", sample);
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
}
```

**X-axis behavior:** Continues from the last X value (e.g., if last point was at X=10, new points start at X=11).

### Mode: Replace

Clears the existing series and replaces it with new data.

```cpp
std::vector<float> new_data = {1.0, 2.0, 3.0, 4.0};
ioSave("display://plot:series=data:mode=replace", new_data);
```

**Use case:** Complete dataset updates, refreshing entire plots.

### Mode: RingBuffer

Appends data but maintains a bounded history. When the limit is exceeded, the oldest points are automatically removed.

```cpp
// Keep only the last 1000 points
for(int i = 0; i < 10000; ++i) {
    float value = generateSensorReading();
    std::vector<float> sample = {value};
    ioSave("display://trace:series=signal:mode=ringbuffer", sample);
}
```

**Configuration:** The maximum history size is controlled by `maxHistoryPoints` in the channel's `PlotState` (default: 10,000 points).

**Use case:** Long-running monitoring where you want a sliding window of recent data.

---

## URL Controls Reference

The `display://` URL format is: `display://<channel>:<controls>`

Where `<channel>` is the channel/plot name (required), and `<controls>` are optional colon-separated parameters:

| Control | Values | Description | Example |
|---------|--------|-------------|---------|
| `<channel>` | string | Channel/plot identifier (required, first component) | `display://sensor1:...` |
| `:series=<name>` | string | Series name within the plot (default: "data") | `:series=temperature` |
| `:mode=<mode>` | `Append`, `Replace`, `RingBuffer` | Update mode (default: Append) | `:mode=ringbuffer` |
| `:clearplot` | (flag) | Clear all series before adding | `:clearplot` |
| `:clearseries` | (flag) | Clear this series before adding | `:clearseries` |

### URL Control Examples

```cpp
// Explicit mode specification
ioSave("display://plot1:series=data:mode=replace", data);

// Clear all series, then add new one
ioSave("display://plot1:clearplot:series=newdata", data);

// Clear specific series, then replace
ioSave("display://plot1:clearseries:series=data:mode=replace", data);

// Streaming with ring buffer
ioSave("display://monitor:series=voltage:mode=ringbuffer", sample);
```

---

## Advanced Examples

### Real-Time Sensor Monitoring

```cpp
void monitorSensor() {
    DebugDisplay::ensureStarted({});

    for(int i = 0; i < 5000; ++i) {
        // Read sensor
        float temperature = readTemperatureSensor();
        float humidity = readHumiditySensor();

        // Plot both on same channel
        std::vector<float> temp_sample = {temperature};
        std::vector<float> humid_sample = {humidity};

        ioSave("display://climate:series=temperature:mode=append", temp_sample);
        ioSave("display://climate:series=humidity:mode=append", humid_sample);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
```

### Signal Processing Pipeline

```cpp
void processSignal(const std::vector<float>& input) {
    // Show original signal
    ioSave("display://pipeline:series=input:mode=replace", input);

    // Apply filter
    auto filtered = applyLowPassFilter(input);
    ioSave("display://pipeline:series=filtered:mode=replace", filtered);

    // Compute FFT magnitude
    auto fft = computeFFT(filtered);
    ioSave("display://spectrum:series=magnitude:mode=replace", fft);
}
```

### Multiple Experiment Runs

```cpp
void runExperiment(int experimentId) {
    // Clear previous results
    std::vector<float> dummy;
    ioSave("display://results:clearplot", dummy);

    // Run experiment and collect data
    std::vector<float> results;
    for(int step = 0; step < 100; ++step) {
        float value = performExperimentStep(step);
        results.push_back(value);
    }

    // Plot complete results
    std::string series = "experiment_" + std::to_string(experimentId);
    ioSave("display://results:series=" + series + ":mode=replace", results);
}
```

### Comparative Analysis

```cpp
void compareAlgorithms() {
    std::vector<float> algorithm_a = runAlgorithmA();
    std::vector<float> algorithm_b = runAlgorithmB();
    std::vector<float> algorithm_c = runAlgorithmC();

    // All appear in same plot for easy comparison
    ioSave("display://comparison:series=Algorithm_A:mode=replace", algorithm_a);
    ioSave("display://comparison:series=Algorithm_B:mode=replace", algorithm_b);
    ioSave("display://comparison:series=Algorithm_C:mode=replace", algorithm_c);
}
```

### Bounded Streaming (Ring Buffer)

```cpp
void monitorWithBoundedHistory() {
    // Monitor indefinitely but keep only recent data
    while(isRunning()) {
        float measurement = takeMeasurement();
        std::vector<float> sample = {measurement};

        // Keeps latest 1000 points automatically
        ioSave("display://live:series=data:mode=ringbuffer", sample);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
```

---

## API Reference

### Data Types

The following data types can be plotted directly:

- `std::vector<float>` - Standard C++ vector
- `Array<float, 1>` - Ravl2 1D array

### Type Conversion

Data is automatically converted to `AddSeriesData` commands via registered type converters. The conversion:
1. Extracts Y values from the array/vector
2. Auto-generates X values as indices (0, 1, 2, ...)
3. Filters out non-finite values (NaN, Inf)
4. Creates a command with channel/series/mode from URL

### X-Axis Generation

Currently, X values are auto-generated as sequential indices:
- First point: X=0
- Second point: X=1
- Third point: X=2
- ...

In **Append mode**, X values continue from the last point:
- If series ends at X=99, next append starts at X=100

### Plot State Structure

Each channel maintains a `PlotState` with:
- `series` - Map of series name → `SeriesData`
- `xAxisLabel` - X-axis label text (default: "X")
- `yAxisLabel` - Y-axis label text (default: "Y")
- `autoFitAxes` - Auto-scale axes (default: true)
- `maxHistoryPoints` - Ring buffer limit (default: 10,000)

Each `SeriesData` contains:
- `x` - X-axis data points
- `y` - Y-axis data points
- `label` - Series name for legend
- `color` - RGBA color (0 = use default)
- `lineWidth` - Line thickness
- `showMarkers` - Display point markers

---

## Tips and Best Practices

### Performance

1. **Use Ring Buffer for Long Runs**: If streaming data continuously, use `:mode=ringbuffer` to prevent unbounded memory growth.

2. **Batch Updates When Possible**: Instead of calling `ioSave()` for every single point, accumulate small batches:
   ```cpp
   std::vector<float> batch;
   for(int i = 0; i < 100; ++i) {
       batch.push_back(computeValue(i));
   }
   ioSave("display://data:series=signal:mode=append", batch);
   ```

3. **Limit Update Rate**: For real-time monitoring, update at reasonable rates (e.g., 10-60 Hz) to avoid overwhelming the display thread.

### Organization

1. **Use Descriptive Channel Names**: Makes it easy to identify plots in the Plots panel.
   ```cpp
   ioSave("display://motor_telemetry:series=rpm", data);
   ```

2. **Group Related Data**: Put related series in the same channel for easy comparison.
   ```cpp
   ioSave("display://imu:series=accel_x", x_accel);
   ioSave("display://imu:series=accel_y", y_accel);
   ioSave("display://imu:series=accel_z", z_accel);
   ```

3. **Use Separate Channels for Different Scales**: If data has very different Y-axis ranges, use separate channels.

### Debugging

1. **Clear Before Fresh Runs**: Use `:clearplot` when starting a new experiment to avoid confusion with old data.

2. **Check Plots Panel**: Open the "Plots" window via ImGui docking if it's not visible.

3. **Verify Data**: If a plot doesn't appear, check:
   - Is the display window running? (`DebugDisplay::ensureStarted({})`)
   - Are there any NaN/Inf values being filtered out?
   - Is the channel name correct?

### Interaction

- **Pan**: Click and drag on the plot
- **Zoom**: Mouse wheel
- **Reset View**: Double-click on plot
- **Toggle Series**: Click legend items to show/hide series
- **Collapse Channel**: Click the channel header to collapse/expand

---

## Example Program

Complete example program demonstrating various features:

```cpp
#include "Ravl2/Array.hh"
#include "Ravl2/IO/Save.hh"
#include "Ravl2/Display/DebugDisplay.hh"
#include <cmath>
#include <thread>
#include <chrono>

int main() {
    // Ensure display is running
    Ravl2::DebugDisplay::ensureStarted({});

    // Example 1: Static plot with multiple series
    std::vector<float> x_values, sine_values, cosine_values;
    for(int i = 0; i < 100; ++i) {
        float x = i * 0.1f;
        x_values.push_back(x);
        sine_values.push_back(std::sin(x));
        cosine_values.push_back(std::cos(x));
    }

    ioSave("display://trig:series=sin:mode=replace", sine_values);
    ioSave("display://trig:series=cos:mode=replace", cosine_values);

    // Example 2: Streaming data with append mode
    for(int i = 0; i < 200; ++i) {
        float value = std::sin(i * 0.05f) + (rand() % 100) / 500.0f; // noisy sine
        std::vector<float> sample = {value};
        ioSave("display://stream:series=signal:mode=append", sample);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Example 3: Ring buffer for continuous monitoring
    for(int i = 0; i < 1000; ++i) {
        float value = std::sin(i * 0.1f);
        std::vector<float> sample = {value};
        ioSave("display://monitor:series=bounded:mode=ringbuffer", sample);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Keep window open
    std::cout << "Plots displayed. Press Enter to exit..." << std::endl;
    std::cin.get();

    return 0;
}
```

Build and run:
```bash
cd cmake-build-debug
cmake --build . --target myprogram
./myprogram
```

---

## Troubleshooting

### Plot Not Appearing

**Problem**: Called `ioSave()` but nothing shows in Plots panel.

**Solutions**:
1. Ensure display is started: `DebugDisplay::ensureStarted({});`
2. Check the Plots window is visible (may be collapsed or docked)
3. Verify channel name doesn't have typos
4. Check data isn't all NaN or Inf (filtered out automatically)

### Data Not Updating

**Problem**: First plot appears but subsequent updates don't show.

**Solutions**:
1. Use `:mode=append` or `:mode=replace` explicitly
2. Ensure you're using the same channel and series names
3. Check if the application is actually calling `ioSave()` in the loop

### Performance Issues

**Problem**: Application slows down with lots of plotting.

**Solutions**:
1. Use `:mode=ringbuffer` to bound history
2. Reduce update frequency (e.g., update every 10ms instead of every 1ms)
3. Batch multiple points into single `ioSave()` call
4. Consider using fewer channels/series

### Memory Usage Growing

**Problem**: Memory consumption increases over time.

**Solutions**:
1. Switch to `:mode=ringbuffer` instead of `:mode=append`
2. Reduce `maxHistoryPoints` in PlotState (default: 10,000)
3. Use `:clearplot` periodically to reset old data

---

## See Also

- `IMPLEMENTATION_CHECKLIST.md` - Implementation status and technical details
- `PHASE7_TIMESERIES_PLAN.md` - Detailed design and architecture
- `DebugDisplay_Design.md` - Overall Debug Display system design
- ImPlot documentation: https://github.com/epezent/implot

---

**Last Updated**: 2025-12-14
**Status**: Phase 7 (a-e) Complete - Full functionality available
