
#include "Ravl2/Math/LeastSquares.hh"
#include "Ravl2/Geometry/Conic2d.hh"
#include "Ravl2/Geometry/Ellipse2d.hh"

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
    //cerr << "A=" << A.range(0).size() << " " << A.range(1).size() << " Vec=" << c.size() << "\n";
    VectorT<RealT> result;
    LeastSquaresEq0Mag1(A,result);

    //SPDLOG_INFO("Result1={}", result);
    // --------- Undo normalisation ----------------
    // TODO:- Make this more efficient by expanding out manually.
    Conic2dC<RealT> Cr(result);
    RealT d = scale;
    Matrix<RealT,3,3> Hi(
      {{ d,0,-mean[0] * d},
        {0,d,-mean[1] * d},
        { 0,0,1}});

    // Matrix<RealT,3,3> nC = Hi.TMul(Cr.C()) * Hi;
    Matrix<RealT,3,3> nC = xt::linalg::dot( xt::linalg::dot(xt::transpose(Hi) ,Cr.C()), Hi);
    conic = Conic2dC(nC);
    return 0;
  }

#if 0
  //! @brief Fit ellipse to points.
  //! Based on method presented in 'Numerically Stable Direct Least Squares Fitting of Ellipses'
  //! by Radim Halir and Jan Flusser.

  template<typename RealT>
  bool fitEllipse(Conic2dC<RealT> &conic, const std::vector<Point<RealT,2>> &points) {
    if(points.size() < 5)
      return false;

    // Normalise points.

    Matrix<RealT,3,3> Hi;
    std::vector<Point<RealT,2>> normPoints;
    Normalise(points,normPoints,Hi);

    // Fill in 'D'
    MatrixC D1(points.size(),3);
    for(BufferAccessIter2C<Point<RealT,2>,BufferAccessC<RealT> > it(normPoints,D1);it;it++) {
      const Point<RealT,2> &l = it.data<0>();
      it.data<1>()[0] = sqr(l[0]);
      it.data<1>()[1] = l[0] * l[1];
      it.data<1>()[2] = sqr(l[1]);
    }
    MatrixC D2(points.size(),3);
    for(BufferAccessIter2C<Point<RealT,2>,BufferAccessC<RealT> > it(normPoints,D2);it;it++) {
      const Point<RealT,2> &l = it.data<0>();
      it.data<1>()[0] = l[0];
      it.data<1>()[1] = l[1];
      it.data<1>()[2] = 1;
    }

    MatrixC S1 = D1.TMul(D1);
    MatrixC S2 = D1.TMul(D2);
    MatrixC S3 = D2.TMul(D2);
    S3.InverseIP();
    MatrixC T = S3.MulT(S2) * -1;
    MatrixC M = S1 + S2 * T;
    M = MatrixC(M[2][0]/2,M[2][1]/2,M[2][2]/2,
                -M[1][0],-M[1][1],-M[1][2],
                M[0][0]/2,M[0][1]/2,M[0][2]/2);

    ONDEBUG(std::cerr << "M=" << M << "\n");
    EigenValueC<RealT> evs(M);
    MatrixC eD;
    evs.getD(eD);
    ONDEBUG(std::cerr << "D=" << eD << "\n");
    MatrixC ev = evs.EigenVectors();
    ONDEBUG(std::cerr << "ev=" << ev << "\n");
    VectorC cond =
      VectorC(ev.SliceRow(0)) * VectorC(ev.SliceRow(2)) * 4
      - VectorC(ev.SliceRow(1)) * VectorC(ev.SliceRow(1));

    //cerr << "Cond=" << cond << "\n";
    VectorC a1 = ev.SliceColumn(cond.IndexOfMax());
    VectorC a = a1.Append(T * a1);

    // Undo normalisation.
    Conic2dC Cr(static_cast<const Vector<RealT,6> &>(a));
    Matrix<RealT,3,3> nC = Hi.TMul(Cr.C()) * Hi;
    conic = Conic2dC(nC);

    return true;
  }
#endif

  //: Fit ellipse to points.

  template<typename RealT>
  std::optional<RealT> fit(Ellipse2dC<RealT> &ellipse, const std::vector<Point<RealT,2>> &points) {
    Conic2dC<RealT> conic;
    auto residual = fit(conic,points);
    if(!residual.has_value())
      return residual;
    auto result = toEllipse<RealT>(conic);
    if(!result.has_value())
      return std::nullopt;
    ellipse = residual.value();
    return residual;
  }

}

#undef DODEBUG
#undef ONDEBUG
