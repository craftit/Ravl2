
#include "Ravl2/Math/LeastSquares.hh"
#include "Ravl2/Geometry/Conic2d.hh"
#include "Ravl2/Geometry/Ellipse2d.hh"

#define DODEBUG 1
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
  std::optional<RealT> fitEllipse(Conic2dC<RealT> &conic, const std::vector<Point<RealT,2>> &points) {
    if(points.size() < 5)
      return std::nullopt;

    // Normalise points.

//    Matrix<RealT,3,3> Hi;
//    std::vector<Point<RealT,2>> normPoints;
//    Normalise(points,normPoints,Hi);

    typename MatrixT<RealT>::shape_type sh = {points.size(), 3};
    Tensor<RealT,2> D1 = xt::empty<RealT>(sh);
    Tensor<RealT,2> D2 = xt::empty<RealT>(sh);
    size_t i = 0;
    // Fill in 'D'
    auto [mean,scale] = normalise<RealT,2>(points,[&D1,&D2,&i](const Point<RealT,2> &l) {
        D1(i,0) = sqr(l[0]);
        D1(i,1) = l[0] * l[1];
        D1(i,2) = sqr(l[1]);

        D2(i,0) = l[0];
        D2(i,1) = l[1];
        D2(i,2) = 1;
        i++;
      });
    ONDEBUG(SPDLOG_INFO("Norm Mean={} Scale={}", mean, scale));

    Tensor<RealT,2> S1 = xt::linalg::dot(xt::transpose(D1),D1);
    Tensor<RealT,2> S2 = xt::linalg::dot(xt::transpose(D1),D2);
    Tensor<RealT,2> S3 = xt::linalg::dot(xt::transpose(D2),D2);

    ONDEBUG(std::cerr << "S1=" << S1 << "\n");
    ONDEBUG(std::cerr << "S2=" << S2 << "\n");
    ONDEBUG(std::cerr << "S3=" << S3 << "\n");

    S3 = xt::linalg::inv(S3);
    Tensor<RealT,2> T = xt::linalg::dot(S3,xt::transpose(S2)) * -1;
    Tensor<RealT,2> M = S1 + xt::linalg::dot(S2,T);
    M = Tensor<RealT,2>({{M(2,0)/2,M(2,1)/2,M(2,2)/2},
                         {-M(1,0),-M(1,1),-M(1,2)},
                         {M(0,0)/2,M(0,1)/2,M(0,2)/2}});

    ONDEBUG(std::cerr << "M=" << M << "\n");
//    EigenValueC<RealT> evs(M);
//    Tensor<RealT,2> eD;
//    evs.getD(eD);
//    ONDEBUG(std::cerr << "D=" << eD << "\n");
//    Tensor<RealT,2> ev = evs.EigenVectors();

    auto [ed,ev] = xt::linalg::eigh(M);

    ONDEBUG(std::cerr << "ev=" << ev << "\n");
    ONDEBUG(std::cerr << "ed=" << ed << "\n");
//    VectorT<RealT> cond =
//      VectorT<RealT>(ev.SliceRow(0)) * VectorT<RealT>(ev.SliceRow(2)) * 4
//      - VectorT<RealT>(ev.SliceRow(1)) * VectorT<RealT>(ev.SliceRow(1));

    auto cond = xt::eval(xt::view(ev, 0, xt::all()) * xt::view(ev, 2, xt::all()) * 4 - xt::view(ev, 1, xt::all()) * xt::view(ev, 1, xt::all()));

    ONDEBUG(std::cerr << "Cond=" << cond << "\n");
//    VectorT<RealT> a1 = ev.SliceColumn(cond.IndexOfMax());
//    VectorT<RealT> a = a1.Append(T * a1);

    Vector<RealT,3> a1 = xt::view(ev, xt::all(), xt::argmax(cond)());
    Vector<RealT,3> a2 = xt::linalg::dot(T , a1);
    Vector<RealT,6> a = xt::concatenate(xt::xtuple(a1, a2),1);

    // Undo normalisation.
    Conic2dC Cr(a);
    Matrix<RealT,3,3> Hi(
      {{ scale,0,-mean[0] * scale},
       {0,scale,-mean[1] * scale},
       { 0,0,1}});

    Matrix<RealT,3,3> nC = xt::linalg::dot( xt::linalg::dot(xt::transpose(Hi) ,Cr.C()), Hi);
    conic = Conic2dC(nC);
    return 0;
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
    ellipse = result.value();
    return residual;
  }

}

#undef DODEBUG
#undef ONDEBUG
