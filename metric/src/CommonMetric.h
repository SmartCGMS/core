/**
* Glucose Prediction
* https://diabetes.zcu.cz/
*
* Author: Tomas Koutny (txkoutny@kiv.zcu.cz)
*/


#pragma once

#include "../../../common/iface/SolverIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include <vector>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

struct TDifference_Point {
	double expected;
	double calculated;
	double datetime;		//datetime when the level was measured
};

struct TProcessed_Difference {
	TDifference_Point raw;
	double difference;
};

class CCommon_Metric : public virtual glucose::IMetric, public virtual refcnt::CReferenced {
protected:
	size_t mAll_Levels_Count = 0;
	const glucose::TMetric_Parameters mParameters;
	std::vector<TProcessed_Difference> mDifferences;	
protected:
	virtual double Do_Calculate_Metric() = 0;
	//Particular metrics should only override this method and no else method
public:
	CCommon_Metric(const glucose::TMetric_Parameters &params);
	virtual ~CCommon_Metric() {};

	virtual HRESULT IfaceCalling Accumulate(const double *times, const double *reference, const double *calculated, const size_t count) final;
	virtual HRESULT IfaceCalling Reset() final;
	virtual HRESULT IfaceCalling Calculate(double *metric, size_t *levels_accumulated, size_t levels_required) final;
	virtual HRESULT IfaceCalling Get_Parameters(glucose::TMetric_Parameters *parameters) final;
};

#pragma warning( pop )