
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