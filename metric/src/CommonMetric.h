/**
* Glucose Prediction
* https://diabetes.zcu.cz/
*
* Author: Tomas Koutny (txkoutny@kiv.zcu.cz)
*/


#pragma once

#include "../..\..\common\iface\SolverIface.h"
#include "../..\..\common\iface\SubjectIface.h"
#include "../..\..\common\rtl\hresult.h"
#include "../..\..\common\rtl\referencedImpl.h"

#include <vector>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

template <typename T>
class CMetricFactory : public IMetricFactory, public virtual CReferenced {
protected:
	TMetricParameters mParameters;
public:
	CMetricFactory(TMetricParameters params) : mParameters(params) {};
	virtual ~CMetricFactory() {};
	virtual HRESULT IfaceCalling CreateCalculator(IMetricCalculator **calc) final {
		return ManufactureObject<T, IMetricCalculator>(calc, mParameters);
	}
};


typedef struct {
	TDifferencePoint raw;
	floattype difference;
} TProcessedDifference;

class CCommonDiffMetric : public IMetricCalculator, public virtual CReferenced {
protected:
	size_t mAllLevelsCount;
	TMetricParameters mParameters;
	std::vector<TProcessedDifference> mDifferences;
	void ProcessDifferencesAsDifferences();
	//processes mDifferences accordingly to mParameters
	void ProcessDifferencesWithISO2013Tolerance();
	void ProcessDifferencesAsSlopes();
protected:
	virtual floattype DoCalculateMetric() = 0;
	//Particular metrics should only override this method and no else method
public:
	CCommonDiffMetric(TMetricParameters params);
	virtual ~CCommonDiffMetric() {};

	virtual HRESULT IfaceCalling Accumulate(TDifferencePoint *differences, size_t count, size_t alllevelscount) final;
	//just pushes differences into mDifferences
	virtual HRESULT IfaceCalling Calculate(floattype *metric, size_t *levelsaccumulated, size_t levelsrequired) final;
	//Calls ProcessDifferences() and then DoCalculateMetric
	virtual HRESULT IfaceCalling Reset() final;
};

#pragma warning( pop )