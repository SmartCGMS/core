#pragma once

#include "../../../../common/iface/ApproxIface.h"
#include "AvgExpApprox.h"

//Epsilon Types - they cannot be declared with extern to allow using them with switch
constexpr size_t etFixedIterations = 1;		//fixed number of iterations
constexpr size_t etMaxAbsDiff = 2;			//maximum aboslute difference of all pointes
constexpr size_t etApproxRelative = 3;		//for each point, maximum difference < (Interpolated-Approximated)*epsilon for interpolated>=approximated
											//else maximum difference < (-Interpolated+Approximated)*epsilon for interpolated<approximated 


extern const double dfYOffset; //some interpolation requires negative values
				//and it is impossible to compute real ln of a negative number
				//100.0 and lower do not work always 

HRESULT AvgExpApproximateData(scgms::WSignal src, CAvgExpApprox **dst, const TAvgExpApproximationParams& params, const TAvgElementary &avgelementary);