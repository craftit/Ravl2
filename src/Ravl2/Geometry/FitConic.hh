
#include "Ravl2/Math/LeastSquares.hh"
#include "Ravl2/Geometry/Conic2d.hh"

#define DODEBUG 0
#if DODEBUG
#define ONDEBUG(x) x
#else
#define ONDEBUG(x)
#endif

namespace Ravl2
{
  //! @brief Fit a conic to a set of points.
  template<typename RealT>
  std::optional<RealT> fit(Conic2dC<RealT> &conic, const std::vector<Point<RealT,2>> &points)
  {
    auto samples = points.size();
    ONDEBUG(std::cerr << "FitConic(), Points=" << points.size() << "\n");
    if(samples < 5) {
      return std::nullopt;
    }

    // ---------- Compute parameters ----------------------
//    if(samples == 5)
//      samples++;
    typename MatrixT<RealT>::shape_type sh = {samples,6};
    Tensor<RealT,2> A(sh);
    size_t i = 0;

    auto [mean,scale] = normalise<RealT,2>(points,[&i,&A](const Point<RealT,2> &p) {
      A(i,0) = sqr(p[0]);
      A(i,1) = p[0] * p[1];
      A(i,2) = sqr(p[1]);
      A(i,3) = p[0];
      A(i,4) = p[1];
      A(i,5) = 1;
      i++;
    });
//    // Duplicate row to avoid problem with SVD.
//    if(samples != points.size()) {
//      i = points.size();
//      A(i) = A(0);
//    }
    //cerr << "A=" << A.range(0).size() << " " << A.range(1).size() << " Vec=" << c.size() << "\n";
    VectorT<RealT> result;
    LeastSquaresEq0Mag1(A,result);

    //cerr << "Result1=" << result << "\n Cr=" << Cr<< "\n";
    // --------- Undo normalisation ----------------
    // TODO:- Make this more efficient by expanding out manually.
    Conic2dC<RealT> Cr(result);
    RealT d = 1/scale;
    Matrix<RealT,3,3> Hi(
      {{ d,0,-mean[0] * d},
        {0,d,-mean[1] * d},
        { 0,0,1}});

    // Matrix<RealT,3,3> nC = Hi.TMul(Cr.C()) * Hi;
    Matrix<RealT,3,3> nC = xt::linalg::dot(xt::transpose(Hi) , xt::linalg::dot(Cr.C(), Hi));
    conic = Conic2dC(nC);
    return 0;
  }

}

#undef DODEBUG
#undef ONDEBUG
