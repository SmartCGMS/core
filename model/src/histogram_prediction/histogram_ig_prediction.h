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

#include "../descriptor.h"

#include <Eigen/Dense>

#include <map>

namespace hist_ig_pred {

	constexpr double Low_Threshold = 3.0;			//mmol/L below which a medical attention is needed
	constexpr double High_Threshold = 13.0;			//dtto above
	
	constexpr double Inv_Band_Size = 3.0;			//abs(Low_Threshold-Band_Size)/Low_Threshold 
	constexpr double Band_Size = 1.0 / Inv_Band_Size; //must imply relative error <= 10% 
	constexpr double Half_Band_Size = 0.5 / Inv_Band_Size;
	constexpr size_t Band_Count = 2 + static_cast<size_t>((High_Threshold - Low_Threshold)*Inv_Band_Size);
	//1 band < mLow_Threshold, n bands in between, and 1 band >=mHigh_Threshold	
	using THistogram = std::array<std::uint8_t, Band_Count>;

	size_t Level_To_Histogram_Index(const double level);
	double Histogram_Index_To_Level(const size_t index);
	void Inc_Histogram(THistogram &histogram, const size_t index);

	using TProbability = Eigen::Array<double, 1, Band_Count, Eigen::RowMajor>;
	TProbability Histogram_To_Probability(const THistogram &histogram);


	constexpr size_t samples_count = 5;

	using TSampling_Delta = Eigen::Array<double, 1, samples_count, Eigen::RowMajor>;
	TSampling_Delta Prediction_Sampling_Delta(const double horizon);

	using T1st = std::array<THistogram, Band_Count>;
	using T2nd = std::array<T1st, Band_Count>;
	using T3rd = std::array<T2nd, Band_Count>;
	using T4th = std::array<T3rd, Band_Count>;
	using T5th = std::array<T4th, Band_Count>;
	/* sixth exceeds 2GB limit per array declaration
	hence leading to too much memory consumption*/
	using T6th = std::array<T5th, Band_Count>;
	
	const double default_horizon = 30.0*scgms::One_Minute;

	enum class NPattern_Dir {
		negative,
		zero,
		positive
	};

	using TVector = Eigen::Array<double, 1, Band_Count, Eigen::RowMajor>;

	class CPattern {
	protected:
		size_t mBand_Idx;
		NPattern_Dir mX2, mX;
		TVector mHistogram;		
	protected:
		NPattern_Dir dbl_2_pat(const double x) const;
	public:
		CPattern(const double current_level, double x2, double x);
		void Update(const double future_level);
		double Level() const;

		bool operator< (const CPattern &other) const;
	};
}


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CHistogram_IG_Prediction : public virtual scgms::CBase_Filter {
protected:
	scgms::SSignal mIst;
	double mDt = hist_ig_pred::default_horizon;
protected:
	std::map<hist_ig_pred::CPattern, hist_ig_pred::CPattern> mPatterns;
	double Update_And_Predict(const double current_time, const double ig_level);//returns prediction at current_time + mDt
private:
	hist_ig_pred::TSampling_Delta mSampling_Delta = hist_ig_pred::Prediction_Sampling_Delta(hist_ig_pred::default_horizon);

	hist_ig_pred::THistogram mIG_Daily;
	std::array<hist_ig_pred::THistogram, 7> mIG_Weekly;
	hist_ig_pred::T5th mIG_Course;

	double Update_And_Predict_X(const double current_time, const double ig_level);//returns prediction at current_time + mDt
protected:	
	// scgms::CBase_Filter iface implementation
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
	virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
	CHistogram_IG_Prediction(scgms::IFilter *output);
	virtual ~CHistogram_IG_Prediction();
};

#pragma warning( pop )