#pragma once
#include <vector>
#include "Ravl2/Eigen.hh"
#include "Ravl2/Types.hh"

namespace Ravl2
{

  //! Multidimensional Gaussian Process surrogate model for Bayesian Optimization
  //!
  //! Implements a Gaussian Process with squared exponential kernel for
  //! modeling unknown functions. Used as a surrogate model in Bayesian optimization.
  template<typename RealT = double>
  class GaussianProcess
  {
  public:
    //! Matrix type for input data and kernel calculations
    using Matrix = Eigen::Matrix<RealT, Eigen::Dynamic, Eigen::Dynamic>;

    //! Vector type for output data and predictions
    using Vector = VectorT<RealT>;

    //! Constructor
    //! @param length_scale Length scale parameter for the squared exponential kernel
    //! @param noise Expected noise on measurements, added to the diagonal of the kernel matrix for numerical stability
    explicit GaussianProcess(RealT length_scale = RealT(1.0), RealT noise = RealT(1e-6));

    //! Fit the GP to training data
    //! @param X Training input points (n_samples x n_features)
    //! @param y Training target values (n_samples)
    void fit(const Matrix &X, const Vector &y);

    //! Predict mean and variance for new points
    //! @param Xtest Test input points (n_test x n_features)
    //! @param mean Output vector to store predicted means (n_test)
    //! @param variance Output vector to store predicted variances (n_test)
    void predict(const Matrix &Xtest, Vector &mean, Vector &variance) const;

    //! Add a new observation for online updates
    //! @param x_new New input point
    //! @param y_new New target value
    void add_observation(const Vector &x_new, RealT y_new);

    //! Get the current length scale parameter
    //! @return Current length scale value
    RealT getLengthScale() const { return mLengthScale; }

    //! Set the length scale parameter
    //! @param length_scale New length scale value
    //! @note A fitted model is refitted with the new value, so predict() stays usable.
    void setLengthScale(RealT length_scale) {
      mLengthScale = length_scale;
      if(mX.rows() > 0)
        refit();
    }

    //! Scale inputs to [0, 1] using these bounds before applying the kernel, so the length scale is a fraction of
    //! the domain rather than in raw units. Call before fit().
    void setInputBounds(const Vector &min, const Vector &max);

  private:
    Matrix mX;        //!< Training input points (scaled, if bounds are set)
    Vector mY;        //!< Training target values (raw)
    Vector mAlpha;    //!< K^-1 * standardised targets
    Eigen::LLT<Matrix> mLLT; //!< Cholesky factor of the kernel matrix
    Matrix mK_inv;    //!< Pseudo-inverse, used only when the Cholesky factorisation fails
    bool mUseLLT = true;
    RealT mYMean = 0; //!< Targets are standardised, so the prior mean is the data mean...
    RealT mYScale = 1;//!< ...and the unit signal variance matches the data spread.
    Vector mInputMin; //!< Input bounds; empty means inputs are used as given
    Vector mInputScale;
    RealT mLengthScale; //!< Length scale parameter for kernel
    RealT mNoise;     //!< Noise parameter for numerical stability

    //! Scale raw inputs by the input bounds, if set.
    Matrix scaleInputs(const Matrix &X) const;

    //! Rebuild the factorisation from mX and mY.
    void refit();

    //! Squared exponential kernel function
    //! @param x1 First input vector
    //! @param x2 Second input vector
    //! @return Kernel value (similarity between inputs)
    RealT kernel(const Vector &x1, const Vector &x2) const;

    //! Compute kernel matrix between two sets of points
    //! @param X1 First set of input points
    //! @param X2 Second set of input points
    //! @return Kernel matrix with similarity values
    Matrix compute_kernel_matrix(const Matrix &X1, const Matrix &X2) const;
  };

  // Explicit instantiation declarations for common types
  extern template class GaussianProcess<float>;
  extern template class GaussianProcess<double>;

}// namespace Ravl2
