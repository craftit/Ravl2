#include "GaussianProcess.hh"
#include <cmath>
#include <limits>
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

  template<typename RealT>
  void GaussianProcess<RealT>::setInputBounds(const Vector &min, const Vector &max)
  {
    if(min.size() != max.size())
      throw std::invalid_argument("GaussianProcess::setInputBounds: min and max sizes differ");
    mInputMin = min;
    mInputScale = (max - min).cwiseMax(std::numeric_limits<RealT>::epsilon()).cwiseInverse();
  }

  template<typename RealT>
  typename GaussianProcess<RealT>::Matrix GaussianProcess<RealT>::scaleInputs(const Matrix &X) const
  {
    if(mInputMin.size() == 0)
      return X;
    if(X.cols() != mInputMin.size())
      throw std::invalid_argument("GaussianProcess: input dimension doesn't match the input bounds");
    return (X.rowwise() - mInputMin.transpose()).array().rowwise() * mInputScale.transpose().array();
  }

  //! Fit the Gaussian Process to training data
  //!
  //! Factorises the kernel matrix for future predictions.
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
    mX = scaleInputs(X);
    mY = y;
    refit();
  }

  template<typename RealT>
  void GaussianProcess<RealT>::refit()
  {
    // Standardise the targets: with a zero prior mean and unit signal variance, raw targets far from 0 made
    // unexplored regions look like improvements and the search never left its first points.
    mYMean = mY.mean();
    const RealT spread = std::sqrt((mY.array() - mYMean).square().mean());
    mYScale = spread > std::numeric_limits<RealT>::epsilon() ? spread : RealT(1);
    const Vector yStd = (mY.array() - mYMean) / mYScale;

    Matrix K = compute_kernel_matrix(mX, mX);
    K.diagonal().array() += mNoise;
    mLLT.compute(K);
    mUseLLT = mLLT.info() == Eigen::Success;
    if(mUseLLT) {
      mAlpha = mLLT.solve(yStd);
      mK_inv.resize(0, 0);
    } else {
      mK_inv = K.completeOrthogonalDecomposition().pseudoInverse();
      mAlpha = mK_inv * yStd;
    }
  }

  //! Predict mean and variance for new test points
  //!
  //! mean = K_s^T * K^-1 * y, var = k(x, x) - K_s^T * K^-1 * K_s (only the diagonal is computed; k(x, x) = 1).
  //!
  //! @param Xtest Test input points (n_test x n_features)
  //! @param mean Output vector to store predicted means (n_test)
  //! @param variance Output vector to store predicted variances (n_test)
  template<typename RealT>
  void GaussianProcess<RealT>::predict(const Matrix &Xtest, Vector &mean, Vector &variance) const
  {
    if(mX.rows() == 0)
      throw std::logic_error("GaussianProcess::predict() called on an unfitted model");
    const Matrix K_s = compute_kernel_matrix(mX, scaleInputs(Xtest));// n_train x n_test
    mean = (K_s.transpose() * mAlpha).array() * mYScale + mYMean;
    Vector reduction;
    if(mUseLLT) {
      const Matrix v = mLLT.matrixL().solve(K_s);
      reduction = v.colwise().squaredNorm().transpose();
    } else {
      reduction = (K_s.array() * (mK_inv * K_s).array()).colwise().sum().transpose();
    }
    variance = ((RealT(1) - reduction.array()).max(RealT(0))) * (mYScale * mYScale);
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
    mX.row(old_rows) = scaleInputs(Matrix(x_new.transpose())).row(0);

    mY.conservativeResize(old_size + 1);
    mY(old_size) = y_new;

    // Refit with new data (mX is already scaled)
    refit();
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
