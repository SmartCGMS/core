#include <vector>
#include <algorithm>
#include <cmath>

#include "AvgExpInterpolation.h"
#include "AvgExpApprox.h"

const double dfYOffset = 10000.0;

struct TControlPoint {
	TGlucoseLevel pt;
	bool phantom;	//!=0 if the point is artifical to improve interpolation
	size_t index;		//for test vector for interpolation
};

using TControlVector = std::vector<TControlPoint>;

HRESULT Enum2Vector(scgms::WSignal src, bool AllowPhantom, TControlVector *dst, const TAvgElementary &avgelementary) {

	/*
		Now, replaced with AvgElementaryFunctions

		const double steepDownRatio = -489.6;	//black magic
		const double steepUpRatio = 489.0;		//black magic

		//Down				Up
		//-1036.8			547.26
		// -518.4			489.6
		//-1814.4			345.6

	*/

	size_t index, j;
	
	

	//make allocation faster by pre-allocating space fo cca expected number of points
	size_t src_cnt;
	HRESULT rc = src.Get_Discrete_Bounds(nullptr, nullptr, &src_cnt);
	if (rc != S_OK) return rc;
	std::vector<double> discrete_times(src_cnt);
	std::vector<double> discrete_levels(src_cnt);

	rc = src.Get_Discrete_Levels(discrete_times.data(), discrete_levels.data(), discrete_times.size(), &src_cnt);
	if (rc != S_OK) return rc;

	dst->reserve(src_cnt);

	index = 0;
	

	for(j=0; j<src_cnt; j++) {
		//just copy the points, do not add any phantom point
		TControlPoint cpt;

		cpt.pt.datetime = discrete_times[j];		
		cpt.pt.level = discrete_levels[j] + dfYOffset;	//rescale from get buffer calls
		cpt.phantom = false;

		dst->push_back(cpt);
	} //for	


	//just to be sure, sort it
	std::sort(dst->begin(), dst->end(),
		[](TControlPoint const & a, TControlPoint const &b){return a.pt.datetime < b.pt.datetime; });


	//OK, this won't be the fastest method, but the addion occur so infrequently
	//that we take it as an excuse for non-optimalized code
	if (AllowPhantom) {
		index = dst->size()-1;
		double k;

		while (index-->1) {
			k = (*dst)[index].pt.level-(*dst)[index-1].pt.level;
			k = k/((*dst)[index].pt.datetime-(*dst)[index-1].pt.datetime);

			if ((k<avgelementary.steepDownRatio) || (k>avgelementary.steepUpRatio)) {
				TControlPoint cpt;

				cpt.pt.datetime = ((*dst)[index].pt.datetime+(*dst)[index-1].pt.datetime)*0.5;

				//cpt.pt.y = ((*dst)[index].pt.y+(*dst)[index-1].pt.y)*0.5;
				//as processes in body goes by exponential function,
				//let's guess a better y with i				
				k = avgelementary.ComputeExpK((*dst)[index - 1].pt, (*dst)[index].pt);
				cpt.pt.level = (*dst)[index-1].pt.level*std::exp(k*(cpt.pt.datetime-(*dst)[index-1].pt.datetime));

				cpt.phantom = true;

				dst->insert(dst->begin()+index, cpt);
			} //if (k<steepDownRatio) ||

		} //while (index-->1) {
	} //if (AllowPhantom) {

	return S_OK;
}

HRESULT AvgExpDistille(TControlVector *src, TAvgExpVector *dst, size_t passes, const TAvgElementary &avgelementary) {

	//let us use pointer arithmetic to reduce overhead associated with overloaded [] operator
	//as the overload results in call proc instead of mov and fld
	TAvgExpPoint *dst1, *dst2, *dst3;
	TControlPoint *src1;

	size_t iSrcCount = src->size();

	size_t	iDstHigh = (iSrcCount-1) << passes;
	//and pre-allocate size for later in place conversion
	dst->resize(iDstHigh+1);

	size_t iDstStride = ((size_t) 1) << passes;

	//first, copy src to the dst
	dst1 = &(*dst)[0];
	src1 = &(*src)[0];
	for (size_t i=0; i<iSrcCount;  i++) {	
		dst1->pt=src1->pt;

		dst1 += iDstStride;
		src1++;
	}


	while (passes-->0) {
		size_t iHelperIndex = iDstStride << 1;

		//Now, compute k-coefficients
		(*dst)[iDstHigh - iDstStride].k = avgelementary.ComputeExpK((*dst)[iDstHigh - iDstStride].pt, (*dst)[iDstHigh].pt);
		//the last segment was a special case
		
		//serial version
		dst2 = &(*dst)[iDstHigh];
		dst1 = dst2 - iHelperIndex;

		for (size_t i = iDstHigh; i > iHelperIndex; i -= iDstStride) {
			dst1->k = avgelementary.ComputeExpK(dst1->pt, dst2->pt);

			dst1 -= iDstStride;
			dst2 -= iDstStride;
		}
		
		

		//With k-coefficients, we can approximate values for the next run

		iHelperIndex = iDstStride >> 1;
		//prepare new X indexes in the dst

		dst1 = &(*dst)[iHelperIndex];
		dst2 = &(*dst)[0];
		dst3 = &(*dst)[iDstStride];

		for (size_t i = iHelperIndex; i < iDstHigh; i += iDstStride) {

			dst1->pt.datetime = (dst2->pt.datetime + dst3->pt.datetime)*0.5;

			dst1 += iDstStride;
			dst2 += iDstStride;
			dst3 += iDstStride;
		}


		//As we do in-place transformation, we need to go from high to low

		//fill the last but one as a special case
		(*dst)[iDstHigh - iHelperIndex].pt.level = avgelementary.ComputeExpKX((*dst)[iDstHigh - iDstStride], (*dst)[iDstHigh - iHelperIndex].pt.datetime);
		
		//serial solution
		dst3 = &(*dst)[iDstHigh - iDstStride * 2];
		dst2 = dst3 + iHelperIndex;
		dst1 = dst3 - iDstStride;

		for (size_t i = iDstHigh - iDstStride * 2; i >= iDstStride; i -= iDstStride) {

			//compute y for the point that's one half of iDstStride to the right
			double ly = avgelementary.ComputeExpKX(*dst1, dst2->pt.datetime);
			double ry = avgelementary.ComputeExpKX(*dst3, dst2->pt.datetime);
			dst2->pt.level = (ly + ry)*0.5;


			//and compute y for the present point
			ly = avgelementary.ComputeExpKX(*dst1, dst3->pt.datetime);
			dst3->pt.level = (ly + dst3->pt.level)*0.5;

			dst1 -= iDstStride;
			dst2 -= iDstStride;
			dst3 -= iDstStride;
		}
		
			

		//fill the first but one as a special case
		(*dst)[iHelperIndex].pt.level = avgelementary.ComputeExpKX((*dst)[0], (*dst)[iHelperIndex].pt.datetime);


		//and prepare stride for the next pass
		iDstStride = iDstStride >> 1;
	} //while (passes-->0) {


	//finally, compute final k-coeficients
	//MComputeExpK((*dst)[iDstHigh-1].pt, (*dst)[iDstHigh].pt, (*dst)[iDstHigh-1].k);

	//dst1 = &(*dst)[iDstHigh-2];
	dst2 = &(*dst)[iDstHigh];
	dst1 = dst2-1;
	dst1->k = avgelementary.ComputeExpK(dst1->pt, dst2->pt);
	dst1--;

	for (size_t i=iDstHigh; i>1; i--) {		
		dst1->k = avgelementary.ComputeExpK(dst1->pt, dst2->pt);

		dst1--;
		dst2--;
	}

	return S_OK;
}

HRESULT Approx2RelativeBounds(TControlVector *EnumSrc, TAvgExpVector *BoundsDst, double epsilon, size_t passes, const TAvgElementary &avgelementary) {

	//each element of the approximated vector will have the following properties:
	//.pt.x  the time of the glucose level
	//.pt.y  the lower bound of the interpolation, where the interpolated glucose level has to be
	//.k     the upper bound

	/*

		For approximated<measured


		measured			.k
		  ^
		  | Epsilon
		  v		 
		  | 1.0 - Epsilon
	   Approximated			.pt.y

	*/

	HRESULT res;

	if ((epsilon<=0.0) || (epsilon>=1.0)) return E_INVALIDARG;

	//get the approximated values, so that we can calculate the bounds using the give epsilon
	CAvgExpApprox approximated{ scgms::WSignal{ nullptr }, avgelementary };
	res = AvgExpDistille(EnumSrc, approximated.getPoints(), passes, avgelementary);
	if (res != S_OK) return res;
		
	size_t srcSize = EnumSrc->size();
	
	for (size_t i=0; i<srcSize; i++) {
		//We need to do this:
		//tmp.pt.x time of the measured level
		//tmp.pt.y the approximated glucose level
		//tmp.k    the fully interpolated, i.e. measured, level

		TAvgExpPoint tmp;
		tmp.pt = (*EnumSrc)[i].pt;
		tmp.k = tmp.pt.level;  //the original, i.e. the fully interpolated, level
	
		res = approximated.GetLevels_Internal(&tmp.pt.datetime, &tmp.pt.level, 1, scgms::apxNo_Derivation);			
		if (res != S_OK) break;

		//add the offset
		tmp.pt.level += dfYOffset;
		//tmp.k += dfYOffset; already adjusted

		//tmp.pt.y is the approximated level now

		//do we need to swap them?
		if (tmp.pt.level>tmp.k) {
			double ftmp = tmp.k;
			tmp.k = tmp.pt.level;
			tmp.pt.level = ftmp;
		}

		//we have the correct upper bound, let's adjust the lower bound by the epsilon

		tmp.pt.level = tmp.k - (tmp.k - tmp.pt.level)*epsilon;

		//finally, push it to save it
		BoundsDst->push_back(tmp);
	}
	
	return res;
}


HRESULT AvgExpInterpolateData(TControlVector *testvector, TAvgExpVector *dst, const TAvgExpApproximationParams& params,
							  TControlVector *iavIn, TControlVector *iavOut,
							  TAvgExpVector *bounds,	//may be NULL, e.g. when approximating 
							  const TAvgElementary &avgelementary) {
						//test vector vector are original values

	TControlVector av;	//approximation vector, i.e. approximated values and indexes into final dst
	size_t i, j, iVectorHigh;
	size_t RemainingIterations = std::max(params.Iterations, static_cast<decltype(params.Iterations)>(1));
		//if we would allow RemainingIterations==0, it would overflow on --!
	bool StopConditionMet = false;
	HRESULT res;

	//let us use pointer arithmetic to reduce overhead associated with overloaded [] operator
	TControlPoint *controlpoints, *testpoints;

	//initialize approximation vector with content of the control vector
	//or, initialize with a precomputed approximation vector
	/*
	if (iavIn) av.assign(iavIn->begin(), iavIn->end());
		else av.assign(testvector->begin(), testvector->end());
		
		But let's do it with std::move!
	*/
	if (iavIn) av = std::move(*iavIn); else
		av.assign(testvector->begin(), testvector->end());	//we need to preserve the test vector


	//get high
	iVectorHigh = av.size()-1;
	//and check for required minimum number of data points
	if (iVectorHigh<2) return E_INVALIDARG;

	//and fill its indexes
	j = (size_t) 1 << params.Passes;
		//by default, 1 is int in C++

	controlpoints = &av[0];
	for (size_t i=0; i<=iVectorHigh; i++) {
		//av[i].index = i*j;

		controlpoints->index = i*j;

		controlpoints++;
	}

	double ftMaxEpsilon = 0.0; //not every program path initiliazes it in the do cycle, after if
	double approxLevel, diff;



	TAvgExpPoint *boundsPtr = nullptr;
	if (bounds) boundsPtr = &((*bounds)[0]);
		
	do {
		//proceed at least once to make at least one approximation

		res = AvgExpDistille(&av, dst, params.Passes, avgelementary);

		if (res == S_OK) {
			//adjust approximation vector by the control vector with respect to approximated values
			//by principle, first and last point never change their y values, so let's don't bother

			controlpoints = &av[1];
			testpoints = &(*testvector)[1];

			TAvgExpPoint *dstPtr = &((*dst)[0]);

			ftMaxEpsilon = 0.0;
			StopConditionMet = params.EpsilonType == etApproxRelative;

			for (i = 1; i <= iVectorHigh; i++) {
				/*if (!av[i].phantom)*/ {
					//actually, it works better if we enforce the phantom points
					//better means lower max epsilon

					//calculate the difference					
					//approxLevel = (*dst)[controlpoints->index].pt.level;
					approxLevel = dstPtr[controlpoints->index].pt.level;
					diff = testpoints->pt.level - approxLevel;


					switch (params.EpsilonType) {
						//max average absolute difference
					case etMaxAbsDiff:     approxLevel = std::fabs(diff);
						if (approxLevel > ftMaxEpsilon) ftMaxEpsilon = approxLevel;
						break;

						//does this level fall into the desired bounds?
					case etApproxRelative: 
						if (boundsPtr != NULL) {
							if (StopConditionMet)
							//										     StopConditionMet = (((*bounds)[i].pt.y<=approxY) &
							//											  				    ((*bounds)[i].k>=approxY));


							//												 StopConditionMet = (((*bounds)[i].k-params->Epsilon<=approxLevel) &
							//												 				     ((*bounds)[i].k+params->Epsilon>=approxLevel));

							StopConditionMet = ((boundsPtr[i].k - params.Epsilon <= approxLevel) &
							(boundsPtr[i].k + params.Epsilon >= approxLevel));

						}//if (boundsPtr != NULL) {
						break;

					}


					//adjust the control point's glucose level
					controlpoints->pt.level += diff;


					controlpoints++;
					testpoints++;
				}
			}


			RemainingIterations--;

			//Test the stop condition, if not set already
			switch (params.EpsilonType) {
			case etFixedIterations: StopConditionMet = RemainingIterations <= 0;
				break;
			case etMaxAbsDiff:	    StopConditionMet = ftMaxEpsilon < params.Epsilon;
				break;

			default:				res = E_INVALIDARG;	//EpsilonType holds an unknown value
				break;
			};

		} //if (res == S_OK)
		else break;

	} while (!StopConditionMet);
	

	///if (iavOut) iavOut->assign(av.begin(), av.end()); - let's use std::move
	if (iavOut) *iavOut = std::move(av);

	return res;
}

HRESULT AvgExpApproximateData(scgms::WSignal src, CAvgExpApprox **dst, const TAvgExpApproximationParams& params, const TAvgElementary &avgelementary) {

	const size_t firstPreCalcPasses = 3;
	const size_t secondPreCalcPasses = 5;
	const size_t firstPreCalcIters = 20;
	const size_t lastCalcIters = 75;

	TControlVector cv;
	TAvgExpVector bounds;
	HRESULT res = Enum2Vector(src, params.Iterations>0 ? true : false, &cv, avgelementary);
	if ((res == S_OK) && (cv.size()>2))
	{
		try {

			CAvgExpApprox *approx = new CAvgExpApprox(src, avgelementary);		
		
			switch (params.EpsilonType) {
				case etApproxRelative:
					//Calculate approximated values first
					res = Approx2RelativeBounds(&cv, &bounds, params.Epsilon, params.Passes, avgelementary);

					if (res == S_OK) {
						res = AvgExpInterpolateData(&cv, approx->getPoints(), params, NULL, NULL, &bounds, avgelementary);
					}

					break; //case etApproxRelative:

				case etMaxAbsDiff: 
					if (params.Epsilon>=0.0)
						res = AvgExpInterpolateData(&cv, approx->getPoints(), params, NULL, NULL, NULL, avgelementary);
					else
						res = E_INVALIDARG;
					break;

				case etFixedIterations:
					if ((params.Passes > 3) && (params.Iterations >= firstPreCalcIters * 2)) {
						//it seems that user want some more precision - this takes time
						//so, sacrifice 20 iterations on a fast pre-calculation

						TControlVector tmp;
						size_t RemainingIterations = params.Iterations;

						TAvgExpApproximationParams tmpAP = params;
						tmpAP.Passes = firstPreCalcPasses;
						tmpAP.Iterations = firstPreCalcIters;

						//do first amount of fast precalculations
						res = AvgExpInterpolateData(&cv, approx->getPoints(), tmpAP, NULL, &tmp, NULL, avgelementary);
						if (res == S_OK) RemainingIterations = RemainingIterations - firstPreCalcIters;

						//do second amount of fast precalculations
						//based on numerical analysis of results, we can do most of the work with 5 passes
						//to reduce error, while needing far less time than with e.g. 10 iterations
						if ((params.Passes > 5) && (RemainingIterations > lastCalcIters) && (res == S_OK)) {

							tmpAP.Passes = secondPreCalcPasses;
							tmpAP.Iterations = RemainingIterations - lastCalcIters;

							res = AvgExpInterpolateData(&cv, approx->getPoints(), tmpAP, &tmp, &tmp, NULL, avgelementary);
							if (res == S_OK) RemainingIterations = lastCalcIters;
						}

						//and finalize
						if (res == S_OK) {
							tmpAP.Passes = params.Passes;
							tmpAP.Iterations = RemainingIterations;
							res = AvgExpInterpolateData(&cv, approx->getPoints(), tmpAP, &tmp, NULL, NULL, avgelementary);
						}
						else
							res = AvgExpInterpolateData(&cv, approx->getPoints(), params, NULL, NULL, NULL, avgelementary);
					}

					break; //case etFixedIterations:

				default:
					res = E_INVALIDARG;
			} //switch (params->EpsilonType) {

			if (res == S_OK)
				*dst = approx;
			else
				delete approx;
		}
		catch (...) {
			res = E_UNEXPECTED;
		}
	} else {
		res = (res == S_OK) ? E_INVALIDARG : res;
		//return E_INVALIDARG to indicate input vector with too little input values
		//if the previous call did not already fail for another reason
	} //if ((res == S_OK) && (cv.size()>2))

	return res;
}
