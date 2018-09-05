/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#pragma once

#include "CommonMetric.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CAbsDiffAvgMetric : public CCommon_Metric {
protected:
  virtual double Do_Calculate_Metric();
public:
	CAbsDiffAvgMetric(const glucose::TMetric_Parameters &params) : CCommon_Metric(params) {};
};

class CAbsDiffMaxMetric : public CCommon_Metric {
protected:
	virtual double Do_Calculate_Metric();
public:
	CAbsDiffMaxMetric(const glucose::TMetric_Parameters &params) : CCommon_Metric(params) {};
};

class CAbsDiffPercentilMetric : public CCommon_Metric {
private:
	double mInvThreshold;
protected:
	virtual double Do_Calculate_Metric();
		//Returns the metric at percentil given by mParameters
public:
	CAbsDiffPercentilMetric(glucose::TMetric_Parameters params);
};


class CAbsDiffThresholdMetric : public CCommon_Metric {
protected:
	virtual double Do_Calculate_Metric();
		//returns the number of levels which have error greater than mParameters.Threshold
public:
	CAbsDiffThresholdMetric(glucose::TMetric_Parameters params) : CCommon_Metric(params) {};
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
	virtual double Do_Calculate_Metric();	
public:
	CLeal2010Metric(glucose::TMetric_Parameters params);
};


class CAICMetric : public CAbsDiffAvgMetric {
protected:
	virtual double Do_Calculate_Metric();
public:
	CAICMetric(glucose::TMetric_Parameters params);
};

class CStdDevMetric : public CAbsDiffPercentilMetric {
protected:
	double mLast_Calculated_Avg = std::numeric_limits<double>::quiet_NaN();			//stores the average calculated with the last call to DoCalculateMetric()
	bool mDo_Bessel_Correction;
	virtual double Do_Calculate_Metric();
public:
	CStdDevMetric(glucose::TMetric_Parameters params) : CAbsDiffPercentilMetric(params), mDo_Bessel_Correction(false) {};
};

class CAvgPlusBesselStdDevMetric : public CStdDevMetric {
	virtual double Do_Calculate_Metric();
public:
	CAvgPlusBesselStdDevMetric(glucose::TMetric_Parameters params) : CStdDevMetric(params) { mDo_Bessel_Correction = true; };
};


class CCrossWalkMetric : public CCommon_Metric {
protected:
	const bool mCross_Measured_With_Calculated_Only = true;
	const bool mCrosswalk4 = false;
	const bool mCompare_To_Measured_Path = true;
	const bool mCalculate_Real_Relative_Difference = false;
	virtual double Do_Calculate_Metric();
public:
	CCrossWalkMetric(glucose::TMetric_Parameters params) : CCommon_Metric(params) {};
};

class CPath_Difference : public CCommon_Metric {
protected:
	virtual double Do_Calculate_Metric();
public:
	CPath_Difference(glucose::TMetric_Parameters params) : CCommon_Metric(params) {};
};

class CIntegralCDFMetric : public CAbsDiffPercentilMetric {
	virtual double Do_Calculate_Metric();
public:
	CIntegralCDFMetric(glucose::TMetric_Parameters params) : CAbsDiffPercentilMetric(params) {};
};


#pragma warning( pop )