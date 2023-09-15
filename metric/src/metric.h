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

#pragma once

#include "CommonMetric.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CAbsDiffAvgMetric : public CCommon_Metric {
protected:
  virtual double Do_Calculate_Metric();
public:
	CAbsDiffAvgMetric(const scgms::TMetric_Parameters& params) : CCommon_Metric(params) {};
};

class CRMSE_Metric : public CAbsDiffAvgMetric {
protected:
	virtual double Do_Calculate_Metric() override final;
public:
	CRMSE_Metric(scgms::TMetric_Parameters& params);
};


class CAbsDiffMaxMetric : public CCommon_Metric {
protected:
	virtual double Do_Calculate_Metric() override final;
public:
	CAbsDiffMaxMetric(const scgms::TMetric_Parameters& params) : CCommon_Metric(params) {};
};

class CAbsDiffPercentilMetric : public CCommon_Metric {
private:
	double mInvThreshold;
protected:
	virtual double Do_Calculate_Metric() override;
		//Returns the metric at percentil given by mParameters
public:
	CAbsDiffPercentilMetric(scgms::TMetric_Parameters& params);
};


class CAbsDiffThresholdMetric : public CCommon_Metric {
protected:
	virtual double Do_Calculate_Metric() override final;
		//returns the number of levels which have error greater than mParameters.Threshold
public:
	CAbsDiffThresholdMetric(scgms::TMetric_Parameters& params) : CCommon_Metric(params) {};
};


class CLeal2010Metric : public CCommon_Metric {
		/* See BestFit from http://www.ncbi.nlm.nih.gov/pmc/articles/PMC2864176/?report=classic
			Yenny Leal at al. Real-Time Glucose Estimation Algorithm for Continuous Glucose Monitoring Using Autoregressive Models
			Best Fit=(1−|BGN−GE||BGN−BGNMMean|)*100%.
			BGN - measured values
			BGNMean - average of BGN values
			GE - calcualted values
			|vector| - || stands for operator of vector lenght aka Euclidean norm
		*/
protected:		
	virtual double Do_Calculate_Metric() override final;
public:
	CLeal2010Metric(scgms::TMetric_Parameters& params);
};


class CAICMetric : public CAbsDiffAvgMetric {
protected:
	virtual double Do_Calculate_Metric() override final;
public:
	CAICMetric(scgms::TMetric_Parameters& params);
};

class CVariance_Metric : public CAbsDiffPercentilMetric {
protected:
	double mLast_Calculated_Avg = std::numeric_limits<double>::quiet_NaN();			//stores the average calculated with the last call to DoCalculateMetric()	
	virtual double Do_Calculate_Metric() override;
public:
	CVariance_Metric(scgms::TMetric_Parameters& params) : CAbsDiffPercentilMetric(params) {};
};

class CStdDevMetric : public CVariance_Metric {
protected:
	virtual double Do_Calculate_Metric() override;
public:
	CStdDevMetric(scgms::TMetric_Parameters& params) : CVariance_Metric(params) {};
};

class CAvgPlusBesselStdDevMetric : public CVariance_Metric {
	virtual double Do_Calculate_Metric() override final;
public:
	CAvgPlusBesselStdDevMetric(scgms::TMetric_Parameters& params) : CVariance_Metric(params) {};
};


class CAvg_Pow_StdDev_Metric : public CVariance_Metric {
	virtual double Do_Calculate_Metric() override final;
public:
	CAvg_Pow_StdDev_Metric(scgms::TMetric_Parameters& params) : CVariance_Metric(params) {};
};


class CCrossWalkMetric : public CCommon_Metric {
protected:
	const bool mCross_Measured_With_Calculated_Only = true;
	const bool mCrosswalk4 = false;
	const bool mCompare_To_Measured_Path = true;
	const bool mCalculate_Real_Relative_Difference = false;
	virtual double Do_Calculate_Metric();
public:
	CCrossWalkMetric(scgms::TMetric_Parameters& params) : CCommon_Metric(params) {};
};

class CPath_Difference : public CCommon_Metric {
protected:
	virtual double Do_Calculate_Metric() override final;
public:
	CPath_Difference(scgms::TMetric_Parameters& params) : CCommon_Metric(params) {};
};

class CIntegralCDFMetric : public CAbsDiffPercentilMetric {
	virtual double Do_Calculate_Metric() override final;
public:
	CIntegralCDFMetric(scgms::TMetric_Parameters params) : CAbsDiffPercentilMetric(params) {};
};

class CExpWeightedDiffAvgPolar_Metric : public CCommon_Metric {
protected:
	virtual double Do_Calculate_Metric();
public:
	CExpWeightedDiffAvgPolar_Metric(const scgms::TMetric_Parameters& params) : CCommon_Metric(params) {};
};


#pragma warning( pop )
