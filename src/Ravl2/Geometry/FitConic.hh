
#include "Ravl2/Math/LeastSquares.hh"
#include "Ravl2/Geometry/Conic2d.hh"

namespace Ravl2
{
  //! @brief Fit a conic to a set of points.
  template<typename RealT>
  RealT fit(Conic2dC<RealT> &conic, const std::vector<Point<RealT,2>> &points)
  {
    unsigned samples = points.size();
    ONDEBUG(std::cerr << "FitConic(), Points=" << points.size() << "\n");
    if(samples < 5) {
      SPDLOG_ERROR("ERROR: Need 5 points or more to fit Conic2dC.");
      throw std::runtime_error("ERROR: Need 5 points or more to fit Conic2dC.");
    }
    // ---------- Compute normalisation for points ----------------------
    Matrix<RealT,3,3> Hi;
    std::vector<Point<RealT,2>> normPoints;
    normPoints.reserve(points.size());

    Normalise(points,normPoints,Hi);

    // ---------- Compute parameters ----------------------
    if(samples == 5)
      samples++;
    Tensor<RealT,2> A(samples,6);
    int i = 0;
    for(auto const &p : points) {
      A[i][0] = sqr(p[0]);
      A[i][1] = p[0] * p[1];
      A[i][2] = sqr(p[1]);
      A[i][3] = p[0];
      A[i][4] = p[1];
      A[i][5] = 1;
      i++;
    }
    // Duplicate row to avoid problem with current SVD.
    if(samples != points.size()) {
      i = points.size();
      const Point<RealT,2> &p = normPoints[0];  // Normalise point.
      A[i][0] = sqr(p[0]);
      A[i][1] = p[0] * p[1];
      A[i][2] = sqr(p[1]);
      A[i][3] = p[0];
      A[i][4] = p[1];
      A[i][5] = 1;
    }
    //cerr << "A=" << A.range(0).size() << " " << A.range(1).size() << " Vec=" << c.size() << "\n";
    VectorT<RealT> result;
    LeastSquaresEq0Mag1(A,result);

    //cerr << "Result1=" << result << "\n Cr=" << Cr<< "\n";
    // --------- Undo normalisation ----------------
    // TODO:- Make this more efficient by expanding out manually.
    Conic2dC<RealT> Cr(static_cast<const Vector<RealT,6> &>(result));
    Matrix<RealT,3,3> nC = Hi.TMul(Cr.C()) * Hi;
    conic = Conic2dC(nC);
    return 0;
  }


}