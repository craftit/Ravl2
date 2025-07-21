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
    //! @note This invalidates the current model and requires refitting
    void setLengthScale(RealT length_scale) {
      mLengthScale = length_scale;
      // Clear fitted data to force refitting with new parameters
      mK.resize(0, 0);
      mK_inv.resize(0, 0);
    }

  private:
    Matrix mX;        //!< Training input points
    Vector mY;        //!< Training target values
    Matrix mK;        //!< Kernel matrix for training points
    Matrix mK_inv;    //!< Inverse of kernel matrix
    RealT mLengthScale; //!< Length scale parameter for kernel
    RealT mNoise;     //!< Noise parameter for numerical stability

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
