#include "pool.h"

CBuffer_Pool<TVector1D> Vector1D_Pool{ [](TVector1D &vector, const auto minimum_size) { if (vector.cols() < static_cast<int>(minimum_size)) vector.resize(Eigen::NoChange, minimum_size); } };