
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

