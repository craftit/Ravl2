#include "GaussianProcess.hh"
#include <cmath>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <cassert>

namespace Ravl2
{

  //! Construct a Gaussian Process model
  //! @param length_scale Length scale parameter for the kernel
  //! @param noise Noise parameter to ensure numerical stability
  template<typename RealT>
  GaussianProcess<RealT>::GaussianProcess(RealT length_scale, RealT noise)
    : mLengthScale(length_scale),
      mNoise(noise)
  {}

  //! Fit the Gaussian Process to training data
  //!
  //! Computes the kernel matrix and its inverse for future predictions.
  //!
  //! @param X Training input points (n_samples x n_features)
  //! @param y Training target values (n_samples)
  template<typename RealT>
  void GaussianProcess<RealT>::fit(const Matrix &X, const Vector &y)
  {
    if(X.rows() == 0 || y.size() == 0) {
      SPDLOG_ERROR("GaussianProcess::fit() called with empty data: X.rows()={}, y.size()={}", X.rows(), y.size());
      throw std::invalid_argument("Cannot fit GP with empty data");
    }
    if(X.rows() != y.size()) {
      SPDLOG_ERROR("GaussianProcess::fit() called with mismatched dimensions: X.rows()={}, y.size()={}", X.rows(), y.size());
      throw std::invalid_argument("Number of input points must match number of target values");
    }

    mX = X;
    mY = y;
    mK = compute_kernel_matrix(X, X);
    mK.diagonal().array() += mNoise;  // More efficient than adding identity matrix

    // Use Cholesky decomposition for better numerical stability
    Eigen::LLT<Matrix> llt(mK);
    if(llt.info() != Eigen::Success) {
      // Fallback to pseudo-inverse if Cholesky fails
      mK_inv = mK.completeOrthogonalDecomposition().pseudoInverse();
    } else {
      mK_inv = llt.solve(Matrix::Identity(mK.rows(), mK.cols()));
    }
  }

  //! Predict mean and variance for new test points
  //!
  //! Uses the standard GP prediction equations:
  //! mean = K_s^T * K^-1 * y
  //! var = K_ss - K_s^T * K^-1 * K_s
  //!
  //! @param Xtest Test input points (n_test x n_features)
  //! @param mean Output vector to store predicted means (n_test)
  //! @param variance Output vector to store predicted variances (n_test)
  template<typename RealT>
  void GaussianProcess<RealT>::predict(const Matrix &Xtest, Vector &mean, Vector &variance) const
  {
    assert(mX.rows() > 0 && "GaussianProcess::predict() called on unfitted model - must call fit() first");
    Matrix K_s = compute_kernel_matrix(mX, Xtest);    // n_train x n_test
    Matrix K_ss = compute_kernel_matrix(Xtest, Xtest);// n_test x n_test
    mean = K_s.transpose() * mK_inv * mY;
    Matrix v = mK_inv * K_s;
    variance = K_ss.diagonal() - (K_s.transpose() * v).diagonal();
  }

  //! Add a new observation for online updates
  //!
  //! Appends the new data point and refits the model.
  //!
  //! @param x_new New input point
  //! @param y_new New target value
  template<typename RealT>
  void GaussianProcess<RealT>::add_observation(const Vector &x_new, RealT y_new)
  {
    // Use Eigen's conservativeResize for more efficient resizing
    const auto old_rows = mX.rows();
    const auto old_size = mY.size();

    mX.conservativeResize(old_rows + 1, Eigen::NoChange);
    mX.row(old_rows) = x_new.transpose();

    mY.conservativeResize(old_size + 1);
    mY(old_size) = y_new;

    // Refit with new data
    fit(mX, mY);
  }

  //! Squared exponential kernel function
  //!
  //! k(x1, x2) = exp(-||x1 - x2||^2 / (2 * length_scale^2))
  //!
  //! @param x1 First input vector
  //! @param x2 Second input vector
  //! @return Kernel value (similarity between inputs)
  template<typename RealT>
  RealT GaussianProcess<RealT>::kernel(const Vector &x1, const Vector &x2) const
  {
    return std::exp(-(x1 - x2).squaredNorm() / (2 * mLengthScale * mLengthScale));
  }

  //! Compute kernel matrix between two sets of points
  //!
  //! @param X1 First set of input points
  //! @param X2 Second set of input points
  //! @return Kernel matrix with similarity values
  template<typename RealT>
  typename GaussianProcess<RealT>::Matrix GaussianProcess<RealT>::compute_kernel_matrix(const Matrix &X1, const Matrix &X2) const
  {
    Matrix K(X1.rows(), X2.rows());
    for(Eigen::Index i = 0; i < X1.rows(); ++i) {
      for(Eigen::Index j = 0; j < X2.rows(); ++j) {
        K(i, j) = kernel(X1.row(i), X2.row(j));
      }
    }
    return K;
  }

  // Explicit instantiations for common types
  template class GaussianProcess<float>;
  template class GaussianProcess<double>;

}// namespace Ravl2
