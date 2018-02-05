/**
* Glucose Prediction
* https://diabetes.zcu.cz/
*
* Author: Tomas Koutny (txkoutny@kiv.zcu.cz)
*/


#pragma once

#include "CommonMetric.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CAbsDiffAvgMetric : public CCommonDiffMetric, public virtual CReferenced {
protected:
  virtual floattype DoCalculateMetric();
public:
	CAbsDiffAvgMetric(TMetricParameters params) : CCommonDiffMetric(params) {};
};

class CAbsDiffMaxMetric : public CCommonDiffMetric, public virtual CReferenced {
protected:
	virtual floattype DoCalculateMetric();
public:
	CAbsDiffMaxMetric(TMetricParameters params) : CCommonDiffMetric(params) {};
};

class CAbsDiffPercentilMetric : public CCommonDiffMetric, public virtual CReferenced {		
private:
	floattype mInvThreshold;
protected:
	virtual floattype DoCalculateMetric();
	//Returns the metric at percentil given by mParameters
public:
	CAbsDiffPercentilMetric(TMetricParameters params);
};


class CAbsDiffThresholdMetric : public CCommonDiffMetric, public virtual CReferenced {
protected:
	virtual floattype DoCalculateMetric();
		//returns the number of levels which have error greater than mParameters.Threshold
public:
	CAbsDiffThresholdMetric(TMetricParameters params) : CCommonDiffMetric(params) {};
};


class CLeal2010Metric : public CCommonDiffMetric, public virtual CReferenced {
		/* See BestFit from http://www.ncbi.nlm.nih.gov/pmc/articles/PMC2864176/?report=classic
			Yenny Leal at al. Real-Time Glucose Estimation Algorithm for Continuous Glucose Monitoring Using Autoregressive Models
			Best Fit=(1−|BGN−GE||BGN−BGNMMean|)*100%.
			BGN - measured values
			BGNMean - average of BGN values
			GE - calcualted values
			|vector| - || stands for operator of vector lenght aka Euclidean norm
		*/
protected:		
	virtual floattype DoCalculateMetric();	
public:
	CLeal2010Metric(TMetricParameters params);
};


class CAICMetric : public CAbsDiffAvgMetric, public virtual CReferenced {
protected:
	virtual floattype DoCalculateMetric();
public:
	CAICMetric(TMetricParameters params);	
};

class CStdDevMetric : public CAbsDiffPercentilMetric, public virtual CReferenced {
protected:
	floattype mLastCalculatedAvg;			//stores the average calculated with the last call to DoCalculateMetric()
	bool mDoBesselCorrection;
	virtual floattype DoCalculateMetric();
public:
	CStdDevMetric(TMetricParameters params) : CAbsDiffPercentilMetric(params), mDoBesselCorrection(false) {};
};

class CAvgPlusBesselStdDevMetric : public CStdDevMetric, public virtual CReferenced {
	virtual floattype DoCalculateMetric();
public:
	CAvgPlusBesselStdDevMetric(TMetricParameters params) : CStdDevMetric(params) { mDoBesselCorrection = true; };
};


class CCrossWalkMetric : public CCommonDiffMetric, public virtual CReferenced {
protected:
	const bool mCrossMeasuredWithCalculatedOnly = true;
	const bool mCrosswalk4 = false;
	const bool mCompare_To_Measured_Path = true;
	const bool mCalculate_Real_Relative_Difference = false;
	virtual floattype DoCalculateMetric();
public:
	CCrossWalkMetric(TMetricParameters params);
};

class CPath_Difference : public CCommonDiffMetric, public virtual CReferenced {
protected:
	virtual floattype DoCalculateMetric();
public:
	CPath_Difference(TMetricParameters params) : CCommonDiffMetric(params) {};
};

class CIntegralCDFMetric : public CAbsDiffPercentilMetric, public virtual CReferenced {
	virtual floattype DoCalculateMetric();
public:
	CIntegralCDFMetric(TMetricParameters params) : CAbsDiffPercentilMetric(params) {};
};


#pragma warning( pop )