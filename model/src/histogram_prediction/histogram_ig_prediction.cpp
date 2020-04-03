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

#include "histogram_ig_prediction.h"
#include "../descriptor.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/utils/math_utils.h"

#include <numeric>

namespace hist_ig_pred {
	size_t Level_To_Histogram_Index(const double level) {
		if (level >= High_Threshold) return Band_Count - 1;

		const double tmp = level - Low_Threshold;
		if (tmp < 0.0) return 0;

		return static_cast<size_t>(floor(tmp*Inv_Band_Size));
	}

	double Histogram_Index_To_Level(const size_t index) {
		if (index == 0) return Low_Threshold - Half_Band_Size;
		if (index >= Band_Count - 1) return High_Threshold + Half_Band_Size;

		return Low_Threshold + static_cast<double>(index - 1)*Band_Size + Half_Band_Size;
	}

	
	CPattern::CPattern(const double current_level, double x2, double x) :
		mBand_Idx(Level_To_Histogram_Index(current_level)), mX2(dbl_2_pat(x2)), mX(dbl_2_pat(x)) {
		mHistogram.setZero();
	}

	NPattern_Dir CPattern::dbl_2_pat(const double x) const {
		if (x < 0.0) return NPattern_Dir::negative;
		else if (x > 0.0) return NPattern_Dir::positive;
			
		return NPattern_Dir::zero;
	}

	void CPattern::Update(const double future_level) {
		const size_t band_idx = Level_To_Histogram_Index(future_level);
		mHistogram(band_idx) += 1.0;
	}

	double CPattern::Level() const {

		auto find_max = [&]() {
			size_t bi = 0;
			double m = mHistogram(0);
			for (auto i = 1; i < mHistogram.cols(); i++)
				if (m < mHistogram(i)) {
					bi = i;
					m = mHistogram(i);
				}

			return Histogram_Index_To_Level(bi);
		};

		auto find_centroid = [&]() {
			THistogram prob = mHistogram;
			const double mx = prob.maxCoeff();
			if (mx > 0.0) {
				prob /= mx;

				const double area = prob.sum();

				double moments = 0.0;
				for (auto i = 0; i < prob.cols(); i++) {
					moments += prob(i)*static_cast<double>(i);
				}

				const size_t result_idx = static_cast<size_t>(std::round(moments / area));

				return Histogram_Index_To_Level(result_idx);
			}
			else
				return std::numeric_limits<double>::quiet_NaN();
		};


		auto find_bisection = [&]() {
			double sum1 = 0.0;
			double sum2 = mHistogram.sum();
			size_t result_idx = mHistogram.cols() - 1;

			for (auto i = 0; i < mHistogram.cols(); i++) {
				const double val = mHistogram(i);
				sum1 += val;
				sum2 -= val;

				if (sum1 > sum2) {
					result_idx = i;
					break;
				}
			}

			return Histogram_Index_To_Level(result_idx);
		};


		return find_centroid();
	}

	bool CPattern::operator< (const CPattern &other) const {
		if (mBand_Idx != other.mBand_Idx) return mBand_Idx < other.mBand_Idx;
		if (mX2 != other.mX2) return mX2 < other.mX2;
		return mX < other.mX;
	}

}

CHistogram_IG_Prediction::CHistogram_IG_Prediction(scgms::IFilter *output) 
	: scgms::CBase_Filter(output, hist_ig_pred::filter_id) {
}

CHistogram_IG_Prediction::~CHistogram_IG_Prediction() {

}

HRESULT CHistogram_IG_Prediction::Do_Execute(scgms::UDevice_Event event) {
	if ((event.event_code() == scgms::NDevice_Event_Code::Level) &&
		(event.signal_id() == scgms::signal_IG)) {

		const double dev_time = event.device_time();
		const uint64_t seg_id = event.segment_id();
		const double level = event.level();
		
		HRESULT rc = Send(event);
		if (SUCCEEDED(rc)) {			
			const double pred_level = Update_And_Predict(dev_time, level);

			if (std::isnormal(pred_level)) {
				scgms::UDevice_Event pred{ scgms::NDevice_Event_Code::Level };
				pred.device_id() = hist_ig_pred::filter_id;
				pred.level() = pred_level;
				pred.device_time() = dev_time + mDt;
				pred.signal_id() = hist_ig_pred::signal_Histogram_IG_Prediction;
				pred.segment_id() = seg_id;
				rc = Send(pred);
			}
		}

		return rc;
	}
	else
		return Send(event);	
}

HRESULT CHistogram_IG_Prediction::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mDt = configuration.Read_Double(rsDt_Column, mDt);

	mIst = scgms::SSignal{ scgms::STime_Segment{}, scgms::signal_IG };
	return S_OK;
}

double CHistogram_IG_Prediction::Update_And_Predict(const double current_time, const double ig_level) {
	double result = std::numeric_limits<double>::quiet_NaN();
 
	if (SUCCEEDED(mIst->Update_Levels(&current_time, &ig_level, 1))) {

		const std::array<double, 3> times = { current_time - mDt - 10 * scgms::One_Minute,
											  current_time - mDt - 5 * scgms::One_Minute,
											  current_time - mDt - 0 * scgms::One_Minute };
		std::array<double, 3> levels;
		if (mIst->Get_Continuous_Levels(nullptr, times.data(), levels.data(), times.size(), scgms::apxNo_Derivation) == S_OK) {

			bool all_normals = true;
			for (auto &level : levels)
				if (!std::isnormal(level)) {
					all_normals = false;
					break;
				}

			if (all_normals) {
				/*
				   0^2*a + 0*b + c = l1
				   1^2*a + 2*b + c = l2
				   2^2*a + 2*b + c = l3

							 c = l1
				    a +  b + c = l2
				   4a + 2b + c = l3

				    a +  b = l2 - l1;			
				   4a + 2b = l3 - l1	

				   4a + 2l2 - 2l1 - 2a = l3 - l1
				   2a = l3 - l1 -2l2+2l1
				   2a = l3 + l1 - 2l2
				    a= 0.5*(l3+l1) - l2					
				*/
				
				const double x2 = 0.5*(levels[2] + levels[0]) - levels[1];
				const double x = levels[1] - levels[0] - x2;
				hist_ig_pred::CPattern pattern{ levels[2], x2, x };

				auto iter = mPatterns.find(pattern);				
				if (iter == mPatterns.end()) {
					auto x = mPatterns.insert(std::pair< hist_ig_pred::CPattern, hist_ig_pred::CPattern>(  pattern, std::move(pattern)));					
					iter = x.first;
				}
							
				iter->second.Update(ig_level);

				result = iter->second.Level();
			}
		}
	}

	return result;
}