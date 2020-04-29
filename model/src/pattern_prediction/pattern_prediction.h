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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#pragma once

#include "../../../../common/iface/DeviceIface.h"
#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/rtl/Common_Calculated_Signal.h"

#include "../descriptor.h"

#include <Eigen/Dense>

#include <map>

namespace pattern_prediction {

	using THistogram = Eigen::Array<double, 1, Band_Count, Eigen::RowMajor>;
	constexpr size_t mOffset_Count = 6;

	class CPattern {
	protected:
		size_t mBand_Idx;
		NPattern_Dir mX2, mX;
		THistogram mHistogram;		
		THistogram mFailures;
	public:
		CPattern(const size_t current_band, NPattern_Dir x2, NPattern_Dir x);
		void Update(const double future_level);
		double Level() const;

		bool operator< (const CPattern &other) const;

		size_t band_idx() { return mBand_Idx; };
		NPattern_Dir x2() { return mX2; };
		NPattern_Dir x() { return mX; };
		size_t freq() { return static_cast<size_t>(mHistogram.sum()); };
		double stdev() { return std::sqrt((mHistogram - mHistogram.mean()).square().sum() / (mHistogram.size() - 1)); };
	};
}


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CPattern_Classification {
protected:
	pattern_prediction::NPattern_Dir dbl_2_pat(const double x) const;

	struct TDir_Pattern {
		pattern_prediction::NPattern_Dir mX2 = pattern_prediction::NPattern_Dir::zero;
		pattern_prediction::NPattern_Dir mX = pattern_prediction::NPattern_Dir::zero;
	};
protected:
	scgms::SSignal mIst;
	double mDt = 30.0*scgms::One_Minute;	
	
	bool Classify(const double current_time, size_t &band_idx, pattern_prediction::NPattern_Dir &x2, pattern_prediction::NPattern_Dir &x) const;
};

class CPattern_Prediction_Filter : public virtual scgms::CBase_Filter, public CPattern_Classification {
protected:
	std::map<pattern_prediction::CPattern, pattern_prediction::CPattern> mPatterns;
	double Update_And_Predict(const double current_time, const double ig_level);//returns prediction at current_time + mDt
protected:
	const bool mDump_Params = true;
	void Dump_Params();
protected:	
	// scgms::CBase_Filter iface implementation
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
	virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
	CPattern_Prediction_Filter(scgms::IFilter *output);
	virtual ~CPattern_Prediction_Filter();
};

class CPattern_Prediction_Signal : public virtual CCommon_Calculated_Signal, public CPattern_Classification {
public:
	CPattern_Prediction_Signal(scgms::WTime_Segment segment);
	virtual ~CPattern_Prediction_Signal() = default;

	//scgms::ISignal iface
	virtual HRESULT IfaceCalling Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
		const double* times, double* const levels, const size_t count, const size_t derivation_order) const final;
	virtual HRESULT IfaceCalling Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const final;
};


#pragma warning( pop )