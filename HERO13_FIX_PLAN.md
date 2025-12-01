# Hero 13 GPMF Parsing - Fix Plan

**Date**: 2025-12-01
**Status**: Root cause identified - fixes needed

---

## Root Causes Identified

Based on Notes.md analysis, there are **3 critical misunderstandings** in the current parser:

### 1. **TSMP is Sample Count, NOT Microseconds** 🔴 CRITICAL

**Current behavior** (GpmfParser.cc:484):
```cpp
uint32_t tsmpValue = BYTESWAP32(tsmpData[0]);  // TSMP in microseconds
float rate = (dataRepeat - 1) * 1000000.0f / tsmpValue;
```

**Actual Hero 13 format**:
```
TSMP type 'L' size 4 data: 1,    // 1 sample in this packet
GPS9 type '?' samplesize 32 repeat 1  // matches TSMP
```

**Actual Hero 8 format**:
```
TSMP type 'L' size 4 data: 19,   // 19 samples in this packet
GPS5 type 'l' samplesize 20 repeat 19  // matches TSMP
```

**TSMP is the number of samples**, not a time duration!

### 2. **GPS9 is Type '?' (Complex) with TYPE Descriptor** 🔴 CRITICAL

**Hero 13 GPS9 structure**:
```
TYPE type 'c' size 9 data: "lllllllSS"  // 7×long (l) + 2×short (S)
SCAL type 'l' samplesize 4 repeat 9 data: 10000000, 10000000, 1000, 1000, 100, 1,...
GPS9 type '?' samplesize 32 repeat 1 data: 512479835,-6307569,58792,89,8,9374,44547099,297,3,...
```

**Type descriptor breakdown**:
- `'l'` (7 times) = signed 32-bit long
- `'S'` (2 times) = unsigned 16-bit short

**Fields**:
1. `l` - Latitude (raw, scale by SCAL[0]=10000000)
2. `l` - Longitude (raw, scale by SCAL[1]=10000000)
3. `l` - Altitude (raw, scale by SCAL[2]=1000)
4. `l` - 2D Speed (raw, scale by SCAL[3]=1000)
5. `l` - 3D Speed (raw, scale by SCAL[4]=100)
6. `l` - Days since 2000-01-01 (raw, SCAL[5]=1)
7. `l` - Seconds (raw, SCAL[6]=...)
8. `S` - DOP (Dilution of Precision) (raw, scale by SCAL[7])
9. `S` - Fix type (0=no lock, 2=2D, 3=3D) (raw, SCAL[8])

**Current code assumes GPS9 is type 'l' (all int32)**, but it's actually **type '?' (complex) with mixed types**!

### 3. **Complex Types Are Not Parsed** 🔴 CRITICAL

**Current code** (GpmfParser.cc:1098-1105):
```cpp
case GPMF_TYPE_COMPLEX: {
  // Complex types have opaque data - return as hex string or raw info
  nlohmann::json result;
  result["type"] = "complex";
  result["size_bytes"] = GPMF_RawDataSize(stream);
  result["sample_count"] = sampleCount;
  return result;  // <-- DATA NOT PARSED!
}
```

**Should do**: Look for sibling TYPE field and parse according to the type descriptor string.

---

## Impact Analysis

### What Works ✅
- **GPS5 (Hero 8)**: Lat/Lon/Alt/Speed parse correctly (type 'l')
- **GYRO/ACCL (All)**: Parse correctly when type 's' (int16)

### What's Broken ❌
- **GPS9 (Hero 13)**: Treated as type 'l' but actually type '?', parsing wrong bytes
- **Sample rate calculation**: TSMP misinterpreted as microseconds
- **All complex types**: SCEN, HUES, FACE, WNDM, MWET, AALP - ignored
- **Hero 13 metadata**: Most streams return garbage

---

## Fix Strategy

### Fix 1: Correct TSMP Interpretation 🔧 HIGH PRIORITY

**Problem**: TSMP is not a timestamp - it's a sample count that should match the data repeat count.

**Current logic** (lines 476-498):
```cpp
// WRONG: treats TSMP as microseconds
uint32_t tsmpValue = BYTESWAP32(tsmpData[0]);  // e.g., 1, 9, 19, 200
float rate = (dataRepeat - 1) * 1000000.0f / tsmpValue;  // WRONG!
```

**Correct logic**:
```cpp
// TSMP is redundant - it just confirms how many samples are in the packet
// We should look for STMP (arrival time) instead for rate calculation
// For now, fall back to default rates or calculate from frame timestamps
```

**Action**:
1. Remove TSMP-based rate calculation (it's wrong)
2. Use STMP (start time) differences between packets to calculate rate
3. Fall back to default rates (GPS: 10Hz, Gyro/Accel: 200Hz)

### Fix 2: Parse GPS9 as Complex Type 🔧 CRITICAL

**Problem**: GPS9 is type '?' with TYPE="lllllllSS", but code assumes type 'l'.

**Action**: Create new function to parse GPS9 based on TYPE descriptor:

```cpp
void GpmfParser::parseGps9Complex(GPMF_stream *stream, ...) {
  // 1. Find sibling TYPE field
  GPMF_stream tempStream = *stream;
  if (GPMF_FindPrev(&tempStream, MAKEID('T','Y','P','E'), GPMF_CURRENT_LEVEL) != GPMF_OK) {
    // No TYPE - cannot parse complex GPS9
    return;
  }

  // 2. Read TYPE descriptor
  char *typeDesc = static_cast<char*>(GPMF_RawData(&tempStream));
  // Expected: "lllllllSS" (7 longs + 2 shorts)

  // 3. Find SCAL field (9 scale factors)
  tempStream = *stream;
  std::vector<int32_t> scales(9, 1);
  if (GPMF_FindPrev(&tempStream, MAKEID('S','C','A','L'), GPMF_CURRENT_LEVEL) == GPMF_OK) {
    int32_t *scaleData = static_cast<int32_t*>(GPMF_RawData(&tempStream));
    for (int i = 0; i < 9; i++) {
      scales[i] = BYTESWAP32(scaleData[i]);
    }
  }

  // 4. Parse GPS9 data using TYPE descriptor
  uint8_t *rawData = static_cast<uint8_t*>(GPMF_RawData(stream));
  size_t offset = 0;

  for (uint32_t sample = 0; sample < sampleCount; sample++) {
    GpsFix fix;

    // Parse according to "lllllllSS" format
    int32_t *longPtr = reinterpret_cast<int32_t*>(rawData + offset);

    // Fields 0-6: longs (lat, lon, alt, speed2d, speed3d, days, secs)
    double lat = BYTESWAP32(longPtr[0]) / static_cast<double>(scales[0]);
    double lon = BYTESWAP32(longPtr[1]) / static_cast<double>(scales[1]);
    double alt = BYTESWAP32(longPtr[2]) / static_cast<double>(scales[2]);
    float speed2d = BYTESWAP32(longPtr[3]) / static_cast<float>(scales[3]);
    float speed3d = BYTESWAP32(longPtr[4]) / static_cast<float>(scales[4]);
    uint32_t days = BYTESWAP32(longPtr[5]) / scales[5];
    uint32_t secs = BYTESWAP32(longPtr[6]) / scales[6];

    offset += 7 * 4;  // 7 longs = 28 bytes

    // Fields 7-8: shorts (DOP, fix)
    uint16_t *shortPtr = reinterpret_cast<uint16_t*>(rawData + offset);
    uint16_t dop = BYTESWAP16(shortPtr[0]);
    uint16_t fixType = BYTESWAP16(shortPtr[1]);

    offset += 2 * 2;  // 2 shorts = 4 bytes

    // Total: 32 bytes per sample (matches samplesize 32)

    fix.location = GPSCoordinate(lat, lon, alt);
    fix.speed = Point<float, 2>(speed2d, speed3d);
    fix.days = days;
    fix.seconds = secs;
    fix.precision = dop / static_cast<float>(scales[7]);
    fix.fix = fixType;  // No scaling for fix type

    frames.push_back(std::make_shared<Video::MetaDataFrame<GpsFix>>(
      fix, streamId + mNextId++, timestamp));
  }
}
```

### Fix 3: Detect GPS9 vs GPS5 Format 🔧 HIGH PRIORITY

**Action**: Add format detection in processLevel():

```cpp
case MAKEID('G', 'P', 'S', '5'):
case MAKEID('G', 'P', 'S', '9'):
  {
    GPMF_SampleType sampleType = GPMF_Type(levelStream);

    if (sampleType == GPMF_TYPE_COMPLEX || sampleType == GPMF_TYPE_UNKNOWN) {
      // Hero 13 format: GPS9 as complex type with TYPE descriptor
      parseGps9Complex(levelStream, frames, streamId, timestamp);
    } else {
      // Hero 8 format: GPS5/GPS9 as simple 'l' (int32) arrays
      parseGps(levelStream, frames, streamId, timestamp);
    }
  }
  break;
```

### Fix 4: Add General Complex Type Parser 🔧 MEDIUM PRIORITY

**Action**: Create `parseComplexType()` helper that uses TYPE descriptor:

```cpp
nlohmann::json parseComplexTypeData(GPMF_stream *stream) {
  // 1. Find TYPE descriptor
  // 2. Parse data according to type string (e.g., "Ff", "BBB", "lllllllSS")
  // 3. Apply SCAL if present
  // 4. Return structured JSON
}
```

This would handle all complex types: SCEN, HUES, FACE, WNDM, MWET, AALP, etc.

---

## Implementation Priority

1. **Fix 1 (TSMP)**: Remove broken rate calculation, use defaults ⏱️ 30 min
2. **Fix 2 (GPS9 complex)**: Parse GPS9 using TYPE descriptor ⏱️ 2 hours
3. **Fix 3 (Detection)**: Add format detection for GPS5 vs GPS9 ⏱️ 30 min
4. **Fix 4 (General complex)**: Handle all complex types ⏱️ 4 hours

---

## Testing Plan

1. **Hero 13 GPS**: Verify lat/lon/alt/speed/fix are correct
2. **Hero 8 GPS**: Ensure old format still works
3. **Sample rates**: Verify gyro/accel rates are reasonable
4. **Complex types**: Test SCEN, HUES, etc. parsing

---

## Example: Correcting the Current Bug

**Current output** (wrong):
```
GPS Fix #1:
  Fix type: 512478383  <-- Reading wrong bytes!
```

**After fix**:
```
GPS Fix #1:
  Fix type: 3  <-- Correct (3D fix)
```

The value `512478383` is actually the **latitude** being read as the fix type because the code doesn't account for the type descriptor changing the field layout.

---

## Questions

1. Should we support both GPS5 and GPS9 formats simultaneously?
2. Do we need full complex type parsing, or just GPS9 for now?
3. Should sample rate calculation be removed entirely or fixed properly?
