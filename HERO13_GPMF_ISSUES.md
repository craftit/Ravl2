# Hero 13 GPMF Parsing Issues

**Date**: 2025-12-01
**Status**: Issues Identified - Investigation Required

---

## Symptoms

Running `ex GoProMetadata` on Hero 13 footage shows:

### 1. **Invalid GPS Fix Type**
```
GPS Fix #1 at 0.000s:
  Location: 51.247837°, -0.631027° (alt: 0.0m)
  Speed: 0.00 m/s (2D), 0.00 m/s (3D)
  Fix type: 512478383  <-- WRONG! Should be 0, 2, or 3
  Satellites: -1
  Precision: 1.4221314
```

### 2. **Invalid Sample Rates**
```
[info] Calculated sample rate from TSMP: 888888.88 Hz (samples=9, tsmp=9μs)
[warning] Unusual GPS sample rate: 888888.9 Hz (typical GoPro: 1-18 Hz)
```

**GPS**: 9 microseconds for 9 samples → 888888 Hz (should be ~10-18 Hz)
**Gyro**: 200 microseconds for 200 samples → 995000 Hz (should be 200-400 Hz)
**Accel**: 200 microseconds for 200 samples → 995000 Hz (should be 200-400 Hz)

### 3. **Correct Location but Garbage Metadata**

The **latitude and longitude appear correct** (51.247837°, -0.631027° is valid UK location), but all other GPS9 fields (fix type, precision, days, seconds) are garbage.

---

## Root Cause Analysis

### Issue 1: TSMP (Timestamp) Parsing

**Location**: `GpmfParser::getSampleRate()` (GpmfParser.cc:478-498)

**Current Code**:
```cpp
uint32_t tsmpValue = BYTESWAP32(tsmpData[0]);  // TSMP in microseconds
```

**Problem**: TSMP value is being read as **9μs** and **200μs**, which are clearly wrong.

**Possible causes**:
1. **Hero 13 uses a different TSMP format** (different encoding or units)
2. **TSMP might not be byte-swapped** on Hero 13 (little-endian instead of big-endian?)
3. **TSMP might be stored as a different type** (e.g., 64-bit instead of 32-bit)

### Issue 2: GPS9 Field Parsing

**Location**: `GpmfParser::parseGps()` (GpmfParser.cc:191-199)

**Current Code**:
```cpp
if(elements == 9) {
  fix.days = BYTESWAP32(rawData[offset + 5]);
  fix.seconds = BYTESWAP32(rawData[offset + 6]);
  int32_t dopRaw = BYTESWAP32(rawData[offset + 7]);
  fix.precision = static_cast<float>(dopRaw) * scale;
  fix.fix = BYTESWAP32(rawData[offset + 8]);  // Fix type: 512478383 (WRONG!)
}
```

**Problem**: The GPS9-specific fields (days, seconds, DOP, fix) are all garbage.

**Analysis**:
- **Lat/Lon/Alt work correctly** (offsets 0-2) → these fields are parsed correctly
- **Speed2D/Speed3D work** (offsets 3-4) → these fields are parsed correctly
- **GPS9 fields fail** (offsets 5-8) → these are either:
  1. In a different format on Hero 13
  2. Not present in Hero 13's GPS9 data
  3. Using a different encoding

---

## Hypothesis

**Hero 13 may have changed the GPMF format in subtle ways:**

1. **TSMP encoding changed** - possibly different units or endianness
2. **GPS9 additional fields removed or reorganized**
3. **SCAL (scale) factor encoding changed**

The fact that **lat/lon/alt parse correctly** suggests the base GPS5 format is unchanged, but GPS9 extensions and timing metadata may have changed.

---

## Debugging Steps

### Step 1: Examine Raw GPMF Data

Use the official GoPro `gpmfdemo` tool to examine the raw GPMF structure:

```bash
cd cmake-build-debug/_deps/gpmf-parser-src
./bin/gpmfdemo ~/Documents/GoPro/Day1/GX010005.MP4
```

This will show the actual GPMF structure and TSMP values as read by the official parser.

### Step 2: Compare with Hero 8 Data

Test the same code on Hero 8 sample footage:

```bash
./examples/exGoProMetadata ./_deps/gpmf-parser-src/samples/hero8.mp4 -m 5 -v
```

If Hero 8 also shows no GPS data, the issue is in our parsing logic, not Hero 13-specific.

### Step 3: Check GPMF Library Version

Verify we're using a recent version of the GPMF parser library that supports Hero 13:

```bash
cd cmake-build-debug/_deps/gpmf-parser-src
git log --oneline | head -10
```

Hero 13 was released in 2024, so the library needs to be from 2024 or later.

### Step 4: Add Raw Data Logging

Temporarily add logging to see the raw values:

```cpp
// In parseGps(), after reading GPS9 fields:
SPDLOG_WARN("GPS9 raw values: days={} secs={} dop={} fix={}",
            rawData[offset + 5], rawData[offset + 6],
            rawData[offset + 7], rawData[offset + 8]);
```

This will show if the values are in the expected positions.

---

## Immediate Workaround

Until the Hero 13 format is understood, we can:

1. **Ignore GPS9-specific fields** - only use GPS5 base fields (lat/lon/alt/speed)
2. **Fall back to default sample rates** - ignore TSMP and use hardcoded rates
3. **Add format detection** - detect Hero 13 and use different parsing logic

**Recommended**: Add a flag to disable GPS9 parsing:

```cpp
// In parseGps():
bool useGPS9Fields = (elements == 9) && (heroVersion < 13);  // Disable for Hero 13

if(useGPS9Fields) {
  // Parse GPS9-specific fields
}
```

---

## Related Files

- `GpmfParser.cc` - GPS/Gyro/Accel parsing logic
- `GpmfParser.hh` - Parser interface
- `exGoProMetadata.cc` - Test program
- Hero 13 test file: `~/Documents/GoPro/Day1/GX010005.MP4`
- Hero 8 test file: `./_deps/gpmf-parser-src/samples/hero8.mp4`

---

## Next Steps

1. **Run official gpmfdemo on Hero 13 footage** to see correct TSMP values
2. **Compare GPMF structure** between Hero 8 and Hero 13
3. **Update parsing logic** based on findings
4. **Add version detection** to handle format differences

---

## Questions for User

1. Do you have access to the official GoPro GPMF documentation for Hero 13?
2. Are there other Hero 13 files we can test with to confirm this is consistent?
3. Should we prioritize getting basic GPS working (ignoring GPS9 fields) or full metadata support?
