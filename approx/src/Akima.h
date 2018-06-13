#pragma once

#include "../../../common/iface/ApproxIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DeviceLib.h"


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CAkima : public glucose::IApproximator, public virtual refcnt::CReferenced {
protected:
	glucose::WSignal mSignal;
	const size_t MINIMUM_NUMBER_POINTS = 5;
	std::vector<double> mInputTimes, mInputLevels, mCoefficients;
	bool Update();
protected:
	/**
	* Computes coefficients of Akima Interpolation.
	* Coefficients are stored in one array containing coefficients stored in
	* column fashion. Eg. first coefficients are stored between 0 and count position (inclusive).
	*/
	void Compute_Coefficients();	//modifies mCoefficients

	/**
	* From mMeasured, it computes differences within three points as needed by Akima interpolation
	*	
	*/
	double Differentiate_Three_Point_Scalar(size_t indexOfDifferentiation,
											size_t indexOfFirstSample,
											size_t indexOfSecondsample,
											size_t indexOfThirdSample);

	/**
	* Interpolate on basis of Hermite's algorithm
	*
	* @param firstDerivatives pointer to output structure with first derivates
	*/
	std::vector<double> Interpolate_Hermite_Scalar(std::vector<double> coefsOfPolynFunc);
public:
public:
	CAkima(glucose::WSignal signal, glucose::IApprox_Parameters_Vector* configuration);
	virtual ~CAkima() {};

	virtual HRESULT IfaceCalling GetLevels(const double* times, double* const levels, const size_t count, const size_t derivation_order) override;
	
};

#pragma warning( pop )