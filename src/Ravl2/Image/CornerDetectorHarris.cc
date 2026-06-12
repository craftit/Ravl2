//
// Created by charles galambos on 12/06/2026.
//

#include "Ravl2/Image/CornerDetectorHarris.hh"
#include "Ravl2/Image/StructureTensor.hh"
#include "Ravl2/Image/PeakDetector.hh"
#include "Ravl2/Math.hh"

namespace Ravl2
{

  CornerDetectorHarris::CornerDetectorHarris(RealT threshold, RealT sigmaD, RealT sigmaI, RealT k)
      : mThreshold(threshold),
        mSigmaD(sigmaD),
        mSigmaI(sigmaI),
        mK(k)
  {
    assert(threshold >= 0 && threshold <= 1);
    assert(sigmaD > 0 && sigmaI > 0);
  }

  std::vector<Corner> CornerDetectorHarris::apply(const Array<uint8_t, 2> &img) const
  {
    Array<RealT, 2> floatImg(img.range());
    for(int r : img.range(0)) {
      for(int c : img.range(1)) {
        floatImg[r][c] = RealT(img[r][c]) / RealT(255);
      }
    }
    return apply(floatImg);
  }

  std::vector<Corner> CornerDetectorHarris::apply(const Array<RealT, 2> &img) const
  {
    std::vector<Corner> corners;
    StructureTensorWorkspace<RealT> ws;
    Array<RealT, 2> rr, rc, cc, response;
    structureTensor(rr, rc, cc, img, mSigmaD, mSigmaI, ws);
    harrisResponse(response, rr, rc, cc, mK);

    RealT maxResponse = 0;
    for(int r : response.range(0)) {
      for(int c : response.range(1)) {
        maxResponse = std::max(maxResponse, response[r][c]);
      }
    }
    if(maxResponse <= 0) {
      return corners;// Flat or edge-only image.
    }
    const RealT minResponse = mThreshold * maxResponse;
    const IndexRange<2> peakRange = response.range().shrink(1);
    for(int r : peakRange[0]) {
      for(int c : peakRange[1]) {
        if(response[r][c] < minResponse) {
          continue;
        }
        const Index<2> at(r, c);
        if(!PeakDetect3Plateau(response, at)) {
          continue;
        }
        const Point<float, 2> location = LocatePeakSubPixel(response, at);
        // sigmaD-scale-normalised gradient at the peak, edgeSobel sign convention.
        const Vector<float, 2> grad = {(img[r + 1][c] - img[r - 1][c]) * RealT(0.5) * mSigmaD,
                                       (img[r][c + 1] - img[r][c - 1]) * RealT(0.5) * mSigmaD};
        const auto level = uint8_t(std::clamp(img[r][c] * RealT(255), RealT(0), RealT(255)));
        corners.emplace_back(location, grad, level);
      }
    }
    return corners;
  }

}// namespace Ravl2
