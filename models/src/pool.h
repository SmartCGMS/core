#pragma once

//just a global variable that represents pools of already allocated memory blocks

#include "../../../common/rtl/Buffer_Pool.h"
#include <Eigen/Dense>

using TVector1D = Eigen::Array<double, 1, Eigen::Dynamic, Eigen::RowMajor>;

extern CBuffer_Pool<TVector1D> Vector1D_Pool;