
# ForeCC codes

 * STMP is the for arrival time of the first sample in the payload.
 * CORI timestamp is the "start of frame"

# Camera Distortion

    "MXCF": ["x1", "x3", "x5", "x7", "x9", "x11", "x13", "x1y2"],
    "MAPX": [1.5805143, -8.1668825, 74.5198746, -451.5002441, 1551.2922363, -2735.5422363, 1923.1572266, -0.1086027 ],
    "MYCF": ["y1", "y3", "y1x2", "y1x4"],
    "MAPY": [1.0238225, -0.1025671, -0.2639930, 0.2979266 ],
    "PYCF": ["r0", "r1", "r2", "r3", "r4", "r5", "r6"],
    "POLY": [0.0000000, 1.8229533, 0.1068096, -0.6631866, 0.3563110, -0.0000000, 0.0000000 ],
    "ZMPL": 0.6549,
    "ARUW": 1.1429,
    "ARWA": 1.7778
    
    The radial distortion (from POLY and PYCF)
    angle = 1.8229533r + 0.1068096r^2 - 0.6631866r^3 + 0.3563110r^4
    
    Same technique for the 2D distortion for HyperView
    
    (from MAPY and MYCF)
    y_out = 1.0238225y - 0.1025671y^3 -0.2639930yx^2 + 0.2979266yx^4
    
    (from MAPX and MXCF) "x3", "x5", "x7", "x9", "x11", "x13", "x1y2
    x_out = 1.5805143x - 8.1668825x^3 + 74.5198746x^5 - 451.5002441x^7 + 1551.2922363x^9 - 2735.5422363x^11 + 1923.1572266x^13 - 0.1086027xy^2
    
    This is not easy. But all the math used is provided.


# Format notes


./cmake-build-debug/gpmf-parser ~/Documents/GoPro/Day2/GX010008.MP4 -g
```
VIDEO FRAMERATE:
  29.970 with 22818 frames
PAYLOAD TIME:
  0.000 to 0.033 seconds
GPMF STRUCTURE:
  DEVC nest size 1652 
    DVID type 'L' size 4 data: 1,
    DVNM type 'c' size 12 data: "HERO13 Black"
    STRM nest size 116 
      STMP type 'J' size 8 data: 5866,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 13 data: "Accelerometer"
      ORIN type 'c' size 3 data: "ZXY"
      SIUN type 'c' size 4 data: "m/s�"
      SCAL type 's' size 2 data: 417,
      TMPC type 'f' size 4 data: 38.406,
      ACCL type 's' size 6 data: 3434,-1377,1609,
    STRM nest size 116 
      STMP type 'J' size 8 data: 5866,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 9 data: "Gyroscope"
      ORIN type 'c' size 3 data: "ZXY"
      SIUN type 'c' size 5 data: "rad/s"
      SCAL type 's' size 2 data: 939,
      TMPC type 'f' size 4 data: 38.406,
      GYRO type 's' size 6 data: 49,57,-23,
    STRM nest size 92 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 29 data: "Exposure time (shutter speed)"
      SIUN type 'c' size 1 data: "s"
      SHUT type 'f' size 4 data: 0.001,
    STRM nest size 84 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 34 data: "White Balance temperature (Kelvin)"
      WBAL type 'S' size 2 data: 4954,
    STRM nest size 80 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 23 data: "White Balance RGB gains"
      WRGB type 'f' size 12 data: 2.074,1.000,1.859,
    STRM nest size 60 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 10 data: "Sensor ISO"
      ISOE type 'S' size 2 data: 384,
    STRM nest size 68 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 17 data: "Average luminance"
      YAVG type 'B' size 1 data: 142,
    STRM nest size 64 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 16 data: "Image uniformity"
      UNIF type 'f' size 4 data: 0.000,
    STRM nest size 244 
      STMP type 'J' size 8 data: 69783,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 53 data: "GPS (Lat., Long., Alt., 2D, 3D, days, secs, DOP, fix)"
      UNIT type 'c' samplesize 3 repeat 9 data: "deg", "deg", "m", ...
      TYPE type 'c' size 9 data: "lllllllSS"
      SCAL type 'l' samplesize 4 repeat 9 data: 10000000, 10000000, 1000, 1000, 100, 1,...
      GPSA type 'F' size 4 data: MSLV,
      GPS9 type '?' samplesize 32 repeat 1 data: 512479835,-6307569,58792,89,8,9374,44547099,297,3,...
    STRM nest size 84 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 17 data: "CameraOrientation"
      SCAL type 's' size 2 data: 32767,
      CORI type 's' size 8 data: 32766,49,42,-29,
    STRM nest size 80 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 16 data: "ImageOrientation"
      SCAL type 's' size 2 data: 32767,
      IORI type 's' size 8 data: 32749,-655,783,-267,
    STRM nest size 64 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 14 data: "Frames Skipped"
      FSKP type 'B' size 1 data: 0,
    STRM nest size 80 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 14 data: "Gravity Vector"
      SCAL type 's' size 2 data: 32767,
      GRAV type 's' size 6 data: -10946,27385,14278,
    STRM nest size 72 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 1,
      STNM type 'c' size 22 data: "Compression Score Main"
      CSCM type 'B' size 2 data: 148,148,
    STRM nest size 196 
      STMP type 'J' size 8 data: 39158,
      TSMP type 'L' size 4 data: 2,
      LOGS type 'B' size 65 data: 1, 150, 214, 91, 185, 53,...
      LOGS type 'B' size 81 data: 1, 6, 183, 202, 40, 189,...
SCALED DATA:
  ACCL 8.235m/s�, -3.302m/s�, 3.859m/s�, 
COMPUTED SAMPLERATES:
  ACCL sampling rate = 29.976029Hz (time -0.000000 to 761.241585)",
  GYRO sampling rate = 29.976029Hz (time -0.000000 to 761.241585)",
  GPS9 sampling rate = 29.976029Hz (time -0.000000 to 761.2
```    


./cmake-build-debug/gpmf-parser samples/hero8.mp4 -g

Part of the output:
```
GPMF STRUCTURE:
  DEVC nest size 6660 
    DVID type 'L' size 4 data: 1,
    DVNM type 'c' size 11 data: "HERO8 Black"
    STRM nest size 1376 
      STMP type 'J' size 8 data: 80669,
      TSMP type 'L' size 4 data: 202,
      STNM type 'c' size 13 data: "Accelerometer"
      MTRX type 'f' size 36 data: -1.000,0.000,0.000,0.000,-1.000,0.000,0.000,0.000,1.000,...
      ORIN type 'c' size 3 data: "zxY"
      ORIO type 'c' size 3 data: "ZXY"
      SIUN type 'c' size 4 data: "m/s�"
      SCAL type 's' size 2 data: 417,
      TMPC type 'f' size 4 data: 32.783,
      ACCL type 's' samplesize 6 repeat 202 data: -4100,-433,-22, -4180,-335,24, -4209,-364,132,...
    STRM nest size 1376 
      STMP type 'J' size 8 data: 80669,
      TSMP type 'L' size 4 data: 202,
      STNM type 'c' size 9 data: "Gyroscope"
      MTRX type 'f' size 36 data: -1.000,0.000,0.000,0.000,-1.000,0.000,0.000,0.000,1.000,...
      ORIN type 'c' size 3 data: "zxY"
      ORIO type 'c' size 3 data: "ZXY"
      SIUN type 'c' size 5 data: "rad/s"
      SCAL type 's' size 2 data: 939,
      TMPC type 'f' size 4 data: 32.783,
      GYRO type 's' samplesize 6 repeat 202 data: -35,66,1, -11,81,-15, 2,83,-33,...
    STRM nest size 208 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 30,
      STNM type 'c' size 29 data: "Exposure time (shutter speed)"
      SIUN type 'c' size 1 data: "s"
      SHUT type 'f' samplesize 4 repeat 30 data: 0.001, 0.001, 0.001, 0.001, 0.001, 0.001,...
    STRM nest size 100 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 9,
      STNM type 'c' size 34 data: "White Balance temperature (Kelvin)"
      WBAL type 'S' samplesize 2 repeat 9 data: 4922, 4922, 4922, 4922, 4922, 4905,...
    STRM nest size 176 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 9,
      STNM type 'c' size 23 data: "White Balance RGB gains"
      WRGB type 'f' samplesize 12 repeat 9 data: 1.723,1.000,1.941, 1.723,1.000,1.938, 1.723,1.000,1.938,...
    STRM nest size 116 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 30,
      STNM type 'c' size 10 data: "Sensor ISO"
      ISOE type 'S' samplesize 2 repeat 30 data: 204, 204, 203, 203, 200, 197,...
    STRM nest size 76 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 9,
      STNM type 'c' size 17 data: "Average luminance"
      YAVG type 'B' size 9 data: 108, 108, 108, 107, 107, 106,...
    STRM nest size 96 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 9,
      STNM type 'c' size 16 data: "Image uniformity"
      UNIF type 'f' samplesize 4 repeat 9 data: 0.344, 0.344, 0.344, 0.372, 0.372, 0.372,...
    STRM nest size 604 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 9,
      STNM type 'c' size 52 data: "Scene classification[[CLASSIFIER_FOUR_CC,prob], ...]"
      TYPE type 'c' size 2 data: "Ff"
      SCEN type '?' samplesize 8 repeat 6 data: SNOW,0.110, URBA,0.290, INDO,0.330, WATR,0.060,...
      SCEN type '?' samplesize 8 repeat 6 data: SNOW,0.150, URBA,0.340, INDO,0.230, WATR,0.070,...
      SCEN type '?' samplesize 8 repeat 6 data: SNOW,0.140, URBA,0.350, INDO,0.200, WATR,0.050,...
      SCEN type '?' samplesize 8 repeat 6 data: SNOW,0.180, URBA,0.340, INDO,0.190, WATR,0.040,...
      SCEN type '?' samplesize 8 repeat 6 data: SNOW,0.140, URBA,0.300, INDO,0.210, WATR,0.110,...
      SCEN type '?' samplesize 8 repeat 6 data: SNOW,0.160, URBA,0.370, INDO,0.170, WATR,0.120,...
      SCEN type '?' samplesize 8 repeat 6 data: SNOW,0.140, URBA,0.220, INDO,0.250, WATR,0.140,...
      SCEN type '?' samplesize 8 repeat 6 data: SNOW,0.160, URBA,0.260, INDO,0.290, WATR,0.070,...
      SCEN type '?' samplesize 8 repeat 6 data: SNOW,0.200, URBA,0.250, INDO,0.240, WATR,0.080,...
    STRM nest size 228 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 9,
      STNM type 'c' size 35 data: "Predominant hue[[hue, weight], ...]"
      TYPE type 'c' size 2 data: "BB"
      HUES type '?' samplesize 2 repeat 3 data: 125,155, 86,60, 24,21,
      HUES type '?' samplesize 2 repeat 3 data: 125,154, 86,61, 24,21,
      HUES type '?' samplesize 2 repeat 3 data: 125,154, 86,60, 24,22,
      HUES type '?' samplesize 2 repeat 3 data: 125,152, 86,61, 24,22,
      HUES type '?' samplesize 2 repeat 3 data: 125,151, 86,62, 24,22,
      HUES type '?' samplesize 2 repeat 3 data: 125,150, 86,63, 24,22,
      HUES type '?' samplesize 2 repeat 3 data: 125,148, 86,64, 24,22,
      HUES type '?' samplesize 2 repeat 3 data: 125,145, 86,68, 24,20,
      HUES type '?' samplesize 2 repeat 3 data: 125,143, 86,69, 24,21,
    STRM nest size 320 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 30,
      STNM type 'c' size 28 data: "Face Coordinates and details"
      TYPE type 'c' size 7 data: "Lffffff"
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
      FACE type '?' samplesize 28 repeat 0 
    STRM nest size 568 
      STMP type 'J' size 8 data: 18164,
      TSMP type 'L' size 4 data: 19,
      STNM type 'c' size 43 data: "GPS (Lat., Long., Alt., 2D speed, 3D speed)"
      GPSF type 'L' size 4 data: 0,
      GPSU type 'U' size 16 data: "191118234208.645",
      GPSP type 'S' size 2 data: 9999,
      UNIT type 'c' samplesize 3 repeat 5 data: "deg", "deg", "m", ...
      SCAL type 'l' samplesize 4 repeat 5 data: 10000000, 10000000, 1000, 1000, 100,
      GPS5 type 'l' samplesize 20 repeat 19 data: 420266244,-1292943386,9540240,0,0, 420264665,-1292940227,9540182,0,0,...
    STRM nest size 308 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 29,
      STNM type 'c' size 17 data: "CameraOrientation"
      SCAL type 's' size 2 data: 32767,
      CORI type 's' samplesize 8 repeat 29 data: 32766,46,69,34, 32766,-3,29,-7,...
    STRM nest size 304 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 29,
      STNM type 'c' size 16 data: "ImageOrientation"
      SCAL type 's' size 2 data: 32767,
      IORI type 's' samplesize 8 repeat 29 data: 32766,47,-57,22, 32766,156,-88,94,...
    STRM nest size 248 
      STMP type 'J' size 8 data: 81772,
      TSMP type 'L' size 4 data: 29,
      STNM type 'c' size 14 data: "Gravity Vector"
      SCAL type 's' size 2 data: 32767,
      GRAV type 's' samplesize 6 repeat 29 data: 0,0,0, 0,0,0, 0,0,0,...
    STRM nest size 128 
      STMP type 'J' size 8 data: 100,
      TSMP type 'L' size 4 data: 10,
      STNM type 'c' size 50 data: "Wind Processing[wind_enable, meter_value(0 - 100)]"
      TYPE type 'c' size 2 data: "BB"
      WNDM type '?' samplesize 2 repeat 10 data: 1,99, 1,93, 1,86, 1,77,...
    STRM nest size 136 
      STMP type 'J' size 8 data: 100,
      TSMP type 'L' size 4 data: 10,
      STNM type 'c' size 48 data: "Microphone is Wet[mic_wet, all_mics, confidence]"
      TYPE type 'c' size 3 data: "BBB"
      MWET type '?' samplesize 3 repeat 10 data: 0,1,0, 0,1,0, 0,1,0,...
    STRM nest size 116 
      STMP type 'J' size 8 data: 100,
      TSMP type 'L' size 4 data: 10,
      STNM type 'c' size 38 data: "AGC audio level[rms_level ,peak_level]"
      TYPE type 'c' size 2 data: "bb"
      AALP type '?' samplesize 2 repeat 10 data: 0,-63, 0,-40, -101,-30, -101,-22,...

```
