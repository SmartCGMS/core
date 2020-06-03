#pragma once

#include "../../../../common/iface/ApproxIface.h"
#include "../../../../common/iface/UIIface.h"
#include "../../../../common/rtl/referencedImpl.h"
#include "../../../../common/rtl/DeviceLib.h"

#include <mutex>

#include "AvgElementaryFunctions.h"



struct TAvgExpApproximationParams {
	size_t Passes;
	size_t Iterations;
	size_t EpsilonType;
	double Epsilon;
	double ResamplingStepping;
} ;

typedef struct {
	size_t ApproximationMethod;	// = apxmAverageExponential 
	union {
		TAvgExpApproximationParams avgexp;
	};
} TApproximationParams;


extern const TApproximationParams dfApproximationParams;

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance


class CAvgExpApprox : public scgms::IApproximator, public virtual refcnt::CReferenced {
protected:
protected:
	scgms::WSignal mSignal;

	std::mutex mUpdate_Guard;
	bool Update();
protected:	
	const TApproximationParams mParameters = dfApproximationParams;
	TAvgExpVector vmPoints;
	bool GetLeftPoint(double desiredtime, size_t *index) const;
		//for X returns index of point that is its direct left, aka preceding, neighbor
		//returns false if not found

protected:
	const TAvgElementary mAvgElementary;
public:
	CAvgExpApprox(scgms::WSignal signal, const TAvgElementary &avgelementary);
	virtual ~CAvgExpApprox();

	TAvgExpVector* getPoints();
	HRESULT GetLevels_Internal(const double* times, double* const levels, const size_t count, const size_t derivation_order);

	virtual HRESULT IfaceCalling GetLevels(const double* times, double* const levels, const size_t count, const size_t derivation_order) override;
};

#pragma warning( pop )


