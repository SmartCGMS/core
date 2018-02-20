#include "pool.h"

CBuffer_Pool<TVector1D> Vector1D_Pool{ [](TVector1D &vector, const auto minimum_size) { if (vector.cols() < static_cast<int>(minimum_size)) vector.resize(Eigen::NoChange, minimum_size); } };

Eigen::Map<TVector1D> Map_Double_To_Eigen(const double* vector, const size_t count) {
	return Eigen::Map<TVector1D> { const_cast<double*>(vector), TVector1D::RowsAtCompileTime, static_cast<Eigen::Index>(count) };
}

Eigen::Map<TVector1D> Map_Double_To_Eigen(double* const vector, const size_t count) {	
	return Eigen::Map<TVector1D> { vector, TVector1D::RowsAtCompileTime, static_cast<Eigen::Index>(count) };
}