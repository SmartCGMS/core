/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "AvgExpApprox.h"

#include "../../../../common/rtl/SolverLib.h"

#include "AvgElementaryFunctions.h"
#include "AvgExpInterpolation.h"

#include <algorithm>
#include <vector>
#include <cmath>

const size_t apxmAverageExponential = 1;		//so far, only exp has the derivation implemented
//const size_t apxmAverageLine = 2;			//currently disabled as not derivation is not implemented for the line


const TApproximationParams dfApproximationParams = {
	apxmAverageExponential, //ApproximationMethod
	{	//avgexp
		10, //Passes
		0, // Iterations
		etFixedIterations, // EpsilonType
		0.1, //Epsilon
		scgms::One_Second// ResamplingStepping
	}
};



CAvgExpApprox::CAvgExpApprox(scgms::WSignal signal, const TAvgElementary& avgelementary) : 
	mSignal(signal), mAvgElementary(avgelementary) {	
}



CAvgExpApprox::~CAvgExpApprox() {

}


TAvgExpVector* CAvgExpApprox::getPoints() {
	return &vmPoints;
}

bool CAvgExpApprox::Update() {
	//check the params if they point to the correct structure
	std::lock_guard<std::mutex> local_guard{ mUpdate_Guard };

	size_t srccnt;
	scgms::TBounds time_bounds;
	if (mSignal.Get_Discrete_Bounds(&time_bounds, nullptr, &srccnt) != S_OK)
		return false;

	
	//Get the information we can prior we begin the calculation	
	if (srccnt < 2) return false;


	//It seems like we can proceed with a calculation => go ahead
	CAvgExpApprox *rawapprox;

	//First, let's make a raw approximation	
	if (AvgExpApproximateData(mSignal, &rawapprox, mParameters.avgexp, mAvgElementary) == S_OK) {
		constexpr size_t rbsApproxBufSize = 10240;	//recommended bufer sizes
		//if it succeded, resample the approximation 
		std::vector<double> times(rbsApproxBufSize);
		std::vector<double> levels(rbsApproxBufSize);

		//First, just store the approximated levels with their times at the desired stepping
		vmPoints.clear();
				
		TAvgExpPoint tmplevel;
		tmplevel.pt.datetime = time_bounds.Min;

		auto get_levels = [&times, &levels, this, &rawapprox](const double first_time)->bool {
			times[0] = first_time;
			for (size_t i = 1; i < times.size(); i++)
				times[i] = times[i - 1] + mParameters.avgexp.ResamplingStepping;

			return rawapprox->GetLevels_Internal(times.data(), levels.data(), levels.size(), scgms::apxNo_Derivation) == S_OK;
		};

		while (get_levels(tmplevel.pt.datetime)) {
			for (size_t i = 0; i < levels.size(); i++) 
				if (std::isnormal(levels[i])) 	{
					tmplevel.pt.datetime = times[i];
					tmplevel.pt.level = levels[i] + dfYOffset;
					vmPoints.push_back(tmplevel); 
			}			

			tmplevel.pt.datetime += mParameters.avgexp.ResamplingStepping;
		}
		
		
		//add the last point if we have missed it
		if (time_bounds.Max > vmPoints[vmPoints.size()-1].pt.datetime) {
			tmplevel.pt.datetime = time_bounds.Max;

			{
				std::vector<double> src_times(srccnt);
				std::vector<double> src_levels(srccnt);
				if (mSignal.Get_Discrete_Levels(src_times.data(), src_levels.data(), srccnt, &srccnt) == S_OK) {
					
					tmplevel.pt.level = src_levels[srccnt - 1];
					tmplevel.pt.level += dfYOffset;
					vmPoints.push_back(tmplevel);
				}
			}

			
		}
		

		//Second, recalculate the k-parameter

		//initialize preceeding points, if we have more than 3 points		
		TAvgExpPoint *rpt, *lpt;
		srccnt = vmPoints.size();	//0 and 1 points were detected and handled above
		if (srccnt == 2) {
			lpt = &vmPoints[srccnt - 2];
			rpt = lpt + 1;
			lpt->k = mAvgElementary.ComputeExpK(lpt->pt, rpt->pt);
		} else {
			//more than two levels
			//y1=C*e^(k*x1)
			//y2=C*e^(k*x2)

			//Well, we could approximate at the present time, but that would introduce
			//a time dependent-flowing error, so we put start of each interval as a zero

			//for x1=0
			//C=y1
			//k=(ln y2 - ln y1)/(x2-x1)      for y2>y1, thus positive k
			//k=-(ln y1 - ln y2)/(x2-x1)      for y2<y1, thus negative k
			//otherwise line => k=0

			//thus, the approximation formula is:
			//y=C*e^(k*(x-x1))

			lpt = &vmPoints[0];
			rpt = lpt + 2;
			for (size_t i = 0; i<srccnt - 2; i++) {
				lpt->k = mAvgElementary.ComputeExpK(lpt->pt, rpt->pt);

#ifdef _DEBUG
				_ASSERT(lpt->pt.level>0.0);
#endif

				lpt++;
				rpt++;
			}
		}

		if (srccnt > 1) {
			lpt = &vmPoints[srccnt - 2];
			rpt = lpt + 1;
			lpt->k = mAvgElementary.ComputeExpK(lpt->pt, rpt->pt);
		}


		//And for sanity
		vmPoints[srccnt - 1].k = 0.0;

		//Finally, free the memory used 		
		delete rawapprox;		

		return true;
	}	

	return false;
}

bool CAvgExpApprox::GetLeftPoint(double desiredtime, size_t *index) const {
	size_t i = vmPoints.size();	
	if (i == 0) return false;

	//is the value in range?
	if ((desiredtime >= vmPoints[0].pt.datetime) && (desiredtime <= vmPoints[i - 1].pt.datetime)) {		

		auto thebegin = vmPoints.begin();
		auto theend = vmPoints.end();
		auto it = std::lower_bound(thebegin, theend,
								   desiredtime,
								   [](TAvgExpPoint lhs, double rhs) -> bool { return lhs.pt.datetime < rhs; });

		if (it != theend) {
			*index = it - thebegin;
			return true;
		}
		else
			return false;
	} else 
		return false; //out of range
}

HRESULT IfaceCalling CAvgExpApprox::GetLevels_Internal(const double* times, double* const levels, const size_t count, const size_t derivation_order) {	
	size_t iCachedSize = vmPoints.size();
	if (iCachedSize<2) return S_FALSE;

	size_t filled = 0;

	for (size_t time_idx = 0; time_idx<count; time_idx++) {

		levels[time_idx] = std::numeric_limits<double>::quiet_NaN();	//sanity, if we would get an index that will not satisfy any of the following conditions

		size_t index;
		if (!GetLeftPoint(times[time_idx], &index)) {			
			continue;
		}
	
	
		//let us use pointer arithmetic to reduce overhead associated with overloaded [] operator
		//as the overload results in call proc instead of mov and fld
		const double X = times[time_idx];		

		//take care about special variant, this time the leading point
		if (index == 0) {
			const auto& pt1 = vmPoints[0];
			const auto& pt2 = vmPoints[1];

			//for the leading point, we need to compute k that points to the very next point
			const double leadingK = mAvgElementary.ComputeExpK(pt1.pt, pt2.pt);

			
			//(*points).y = vmPoints[0].pt.y*exp(leadingK*(X-vmPoints[0].pt.x)) * YScaleOut;
			//				(*points).y = (*pt1).pt.y*exp(leadingK*(X-(*pt1).pt.x)) * YScaleOut;
			levels[time_idx] = pt1.pt.level*std::exp(leadingK*(X - pt1.pt.datetime)) - dfYOffset;

			//calculate derivatives as needed
			for (size_t deriter = 0; deriter < derivation_order; deriter++) {
				levels[time_idx] *= leadingK;
			}
			//y = c*exp(k*x)

			filled++;						
		}


		//compute with inside points - i.e. between first and last points exclusive
		if ((index>0) & (index<iCachedSize - 1)) {
			//pt1 = &vmPoints[imPredictedPoint-1];
			const auto& pt2 = vmPoints[index];
			const auto& pt1 = vmPoints[index - 1];

		
			//there are two exponential functions to weight them
			//1. compute Y for the left value - i.e. prior the current point
			const double ly = mAvgElementary.ComputeExpKX(pt1, X);

			//2. compute Y to the right value - i.e. from the current point on
			const double ry = mAvgElementary.ComputeExpKX(pt2, X);

			//and store their average value

				
			//				(*points).y = (ly+ry)*0.5 * YScaleOut;
			levels[time_idx] = (ly + ry)*0.5 - dfYOffset;

			//calculate derivatives as needed
			double dermul = (pt1.k + pt2.k)*0.5;
			for (size_t deriter = 0; deriter < derivation_order; deriter++) {
				levels[time_idx] *= dermul;
			}


			//				for debug
			//				if ((*points).y<0)
			//					break;

			filled++;				
		} 


		//finally, take care about the last segment - i.e. from last but one to the last point
		if ((index>= iCachedSize - 2) && (index < iCachedSize)) {
			const double followingX = vmPoints[iCachedSize - 1].pt.datetime;
			const auto& pt1 = vmPoints[index];

			if (X<=followingX) {//make sure that we did not cross the last known point											
					levels[time_idx] = mAvgElementary.ComputeExpKX(pt1, X);
					//				(*points).y *= YScaleOut;
					levels[time_idx] -= dfYOffset;
					//y = c*exp(k*x)

					for (size_t deriter = 0; deriter < derivation_order; deriter++) {
						levels[time_idx] *= pt1.k;
					}


					filled++;					
				} 
		}		
	}
	
	return filled > 0 ? S_OK : S_FALSE;
	
}

HRESULT IfaceCalling CAvgExpApprox::GetLevels(const double* times, double* const levels, const size_t count, const size_t derivation_order) {
	if (!Update()) return E_FAIL;
	return GetLevels_Internal(times, levels, count, derivation_order);
}