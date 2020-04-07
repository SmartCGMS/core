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
#include "../../../../common/utils/DebugHelper.h"

#include <numeric>
#include <sstream>
#include <iomanip>

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

	
	CPattern::CPattern(const size_t current_band, NPattern_Dir x2, NPattern_Dir x) :
		mBand_Idx(current_band), mX2(x2), mX(x) {
		mHistogram.setZero();
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

hist_ig_pred::NPattern_Dir CHistogram_Classification::dbl_2_pat(const double x) const {
	if (x < 0.0) return hist_ig_pred::NPattern_Dir::negative;
	else if (x > 0.0) return hist_ig_pred::NPattern_Dir::positive;

	return hist_ig_pred::NPattern_Dir::zero;
}


std::array<CHistogram_Classification::TDir_Pattern, 9> CHistogram_Classification::Calculate_Left_Hand_Sum() {
	std::array<CHistogram_Classification::TDir_Pattern, 9> result;
	
	size_t idx = 0;
	//try all combinations of a: -1, 0 and 1
	double a = -1.0;
	do {

		//try all combinations of b: -1, 0 and 1
		double b = -1.0;
		do {

			//calculate the error for this particular combination of and b
			double left_hand_sum = 0.0;
			for (size_t t = 1; t < hist_ig_pred::mOffset_Count; t ++ ) {
				const double tt = static_cast<double>(t);
				left_hand_sum = a * tt*tt + b * tt;
			}
			result[idx].sum = left_hand_sum;
			result[idx].mX2 = dbl_2_pat(a);
			result[idx].mX = dbl_2_pat(b);
			idx++;

			b += 1.0;
		} while (b < 2.0);

		a += 1.0;
	} while (a < 2.0);

	std::sort(result.begin(), result.end(), [](auto a, auto b) {return a.sum < b.sum; });
		
	return result;
}

bool CHistogram_Classification::Classify_Slow_Fast_AUC(const double current_time, size_t &band_idx, hist_ig_pred::NPattern_Dir &x2, hist_ig_pred::NPattern_Dir &x) const {
	bool result = false;

	std::array<double, hist_ig_pred::mOffset_Count> times, levels;
	for (size_t i = 0; i < mOffset.size(); i++)
		times[i] = current_time + mOffset[i];
	if (mIst->Get_Continuous_Levels(nullptr, times.data(), levels.data(), times.size(), scgms::apxNo_Derivation) == S_OK) {

		bool all_normals = true;
		for (auto &level : levels)
			if (!std::isnormal(level)) {
				all_normals = false;
				break;
			}

		if (all_normals) {
			
			//fast channel - all the levels
			{
				double sum = static_cast<double>(levels.size()-1)*levels[0];
				for (size_t i = 1; i < levels.size(); i++)
					sum += levels[i];
				x2 = sum > 0.0 ? hist_ig_pred::NPattern_Dir::positive : hist_ig_pred::NPattern_Dir::negative;
			}

			//fast channel - only three last levels
			{				
				const size_t st_idx = levels.size()-3;
				const double sum = levels[st_idx+1] + levels[st_idx + 2] - 2.0*levels[st_idx];				
				x = sum > 0.0 ? hist_ig_pred::NPattern_Dir::positive : hist_ig_pred::NPattern_Dir::negative;
			}

			band_idx = hist_ig_pred::Level_To_Histogram_Index(levels[hist_ig_pred::mOffset_Count - 1]);

			result = true;
		}
	}

	return result;
}


bool CHistogram_Classification::Classify_Slow_Fast_Line(const double current_time, size_t &band_idx, hist_ig_pred::NPattern_Dir &x2, hist_ig_pred::NPattern_Dir &x) const {
	bool result = false;

	std::array<double, hist_ig_pred::mOffset_Count> times, levels;
	for (size_t i = 0; i < mOffset.size(); i++)
		times[i] = current_time + mOffset[i];
	if (mIst->Get_Continuous_Levels(nullptr, times.data(), levels.data(), times.size(), scgms::apxNo_Derivation) == S_OK) {

		bool all_normals = true;
		for (auto &level : levels)
			if (!std::isnormal(level)) {
				all_normals = false;
				break;
			}

		if (all_normals) {
			//fast channel - all the levels
			{
				const double ds = static_cast<double>(levels.size());

				double left_hand_sum_P = 0.5*ds*(0.0 + (ds - 1.0));
				double left_hand_sum_Z = ds * levels[0];
				double left_hand_sum_N = left_hand_sum_Z - left_hand_sum_P;
				left_hand_sum_P += left_hand_sum_Z;

				double right_hand_sum = 0.0;
				for (size_t i = 0; i < levels.size(); i++)
					right_hand_sum += levels[i];

				x2 = hist_ig_pred::NPattern_Dir::negative;
				double least_diff = fabs(right_hand_sum - left_hand_sum_N);
				double diff = fabs(right_hand_sum - left_hand_sum_Z);
				if (diff < least_diff) {
					least_diff = diff;
					x2 = hist_ig_pred::NPattern_Dir::negative;
				}
				if (least_diff > fabs(right_hand_sum - left_hand_sum_P))
					x2 = hist_ig_pred::NPattern_Dir::positive;
			}


			//fast channel - only three last levels
			{
				double left_hand_sum_P = 0.0 + 1.0 + 2.0;
				double left_hand_sum_Z = 3.0 * levels[levels.size() - 3];
				double left_hand_sum_N = left_hand_sum_Z - left_hand_sum_P;
				left_hand_sum_P += left_hand_sum_Z;

				double right_hand_sum = 0.0;
				for (size_t i = levels.size()-3; i < levels.size(); i++)
					right_hand_sum += levels[i];

				x = hist_ig_pred::NPattern_Dir::negative;
				double least_diff = fabs(right_hand_sum - left_hand_sum_N);
				double diff = fabs(right_hand_sum - left_hand_sum_Z);
				if (diff < least_diff) {
					least_diff = diff;
					x = hist_ig_pred::NPattern_Dir::negative;
				}
				if (least_diff > fabs(right_hand_sum - left_hand_sum_P))
					x = hist_ig_pred::NPattern_Dir::positive;
			}
			


			band_idx = hist_ig_pred::Level_To_Histogram_Index(levels[hist_ig_pred::mOffset_Count - 1]);
			

			result = true;
		}
	}

	return result;
}

bool CHistogram_Classification::Classify_Lookup(const double current_time, size_t &band_idx, hist_ig_pred::NPattern_Dir &x2, hist_ig_pred::NPattern_Dir &x) const {
	bool result = false;

	std::array<double, hist_ig_pred::mOffset_Count> times, levels;
	for (size_t i = 0; i < mOffset.size(); i++)
		times[i] = current_time + mOffset[i];
	if (mIst->Get_Continuous_Levels(nullptr, times.data(), levels.data(), times.size(), scgms::apxNo_Derivation) == S_OK) {

		bool all_normals = true;
		for (auto &level : levels)
			if (!std::isnormal(level)) {
				all_normals = false;
				break;
			}

		if (all_normals) {
			double right_hand_sum = 0.0;
			for (size_t t = 1; t < levels.size(); t++) {
				right_hand_sum += levels[t] - levels[0];
			}

			//let's find the index
			auto lb = std::lower_bound(mLeft_Hand_Sums.begin(), mLeft_Hand_Sums.end(),
				CHistogram_Classification::TDir_Pattern{ right_hand_sum, hist_ig_pred::NPattern_Dir::zero, hist_ig_pred::NPattern_Dir::zero },
				[](auto a, auto b) {return a.sum < b.sum; });
			
			size_t pattern_idx = 0;
			if (lb != mLeft_Hand_Sums.end()) {
				pattern_idx = std::distance(mLeft_Hand_Sums.begin(), lb);
				if (pattern_idx > 0) {
					const size_t prev_pattern_idx = pattern_idx - 1;
					const double diff_prev = std::fabs(right_hand_sum - mLeft_Hand_Sums[prev_pattern_idx].sum);
					const double diff_curr = std::fabs(right_hand_sum - mLeft_Hand_Sums[pattern_idx].sum);

					if (diff_prev < diff_curr)
						pattern_idx = prev_pattern_idx;
				}
			} else
				pattern_idx = mLeft_Hand_Sums.size() - 1;
				


			band_idx = hist_ig_pred::Level_To_Histogram_Index(levels[hist_ig_pred::mOffset_Count-1]);
			x2 = mLeft_Hand_Sums[pattern_idx].mX2;
			x = mLeft_Hand_Sums[pattern_idx].mX;

			result = true;
		}
	}

	return result;
}

bool CHistogram_Classification::Classify_Poly(const double current_time, size_t &band_idx, hist_ig_pred::NPattern_Dir &x2, hist_ig_pred::NPattern_Dir &x) const {
	bool result = false;

	const std::array<double, 3> times = { current_time - 10 * scgms::One_Minute,
										  current_time - 5 * scgms::One_Minute,
										  current_time - 0 * scgms::One_Minute };
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

			const double x2_raw = 0.5*(levels[2] + levels[0]) - levels[1];
			const double x_raw = levels[1] - levels[0] - x2_raw;

			band_idx = hist_ig_pred::Level_To_Histogram_Index(levels[2]);
			x2 = dbl_2_pat(x2_raw);
			x = dbl_2_pat(x_raw);
			result = true;
		}
	}

	return result;
}

bool CHistogram_Classification::Classify(const double current_time, size_t &band_idx, hist_ig_pred::NPattern_Dir &x2, hist_ig_pred::NPattern_Dir &x) const {
	return Classify_Lookup(current_time, band_idx, x2, x);
	return Classify_Poly(current_time, band_idx, x2, x);	
	return Classify_Slow_Fast_AUC(current_time, band_idx, x2, x);
	return Classify_Slow_Fast_Line(current_time, band_idx, x2, x);
}

CHistogram_IG_Prediction_Filter::CHistogram_IG_Prediction_Filter(scgms::IFilter *output) 
	: scgms::CBase_Filter(output, hist_ig_pred::filter_id) {
}

CHistogram_IG_Prediction_Filter::~CHistogram_IG_Prediction_Filter() {
	if (mDump_Params)
		Dump_Params();
}

HRESULT CHistogram_IG_Prediction_Filter::Do_Execute(scgms::UDevice_Event event) {
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

HRESULT CHistogram_IG_Prediction_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mDt = configuration.Read_Double(rsDt_Column, mDt);

	mIst = scgms::SSignal{ scgms::STime_Segment{}, scgms::signal_IG };
	return S_OK;
}

double CHistogram_IG_Prediction_Filter::Update_And_Predict(const double current_time, const double ig_level) {
	double result = std::numeric_limits<double>::quiet_NaN();
 
	if (SUCCEEDED(mIst->Update_Levels(&current_time, &ig_level, 1))) {
		size_t band_idx;
		hist_ig_pred::NPattern_Dir x2, x;
		if (Classify(current_time, band_idx, x2, x)) {
			hist_ig_pred::CPattern pattern{ band_idx, x2, x };

			auto iter = mPatterns.find(pattern);				
			if (iter == mPatterns.end()) {
				auto x = mPatterns.insert(std::pair< hist_ig_pred::CPattern, hist_ig_pred::CPattern>(  pattern, std::move(pattern)));					
				iter = x.first;
			}
							
			iter->second.Update(ig_level);

			result = iter->second.Level();		
		}
	}

	return result;
}

void CHistogram_IG_Prediction_Filter::Dump_Params() {
	std::stringstream def_params, used_rules, unused_rules;
	size_t unused_counter = 0, used_counter = 0;
	const char *dir_chars = "v-^";

	std::array<std::array<size_t, 3>, 3> x2x_freq;
	memset(&x2x_freq, 0, sizeof(x2x_freq));

	auto log_params = [&](std::stringstream &str, size_t &counter, hist_ig_pred::CPattern &pattern) {
		counter++;
		str << "counter " << counter;
		const double lvl = hist_ig_pred::Histogram_Index_To_Level(pattern.band_idx());
		str << "; band index: " << pattern.band_idx();
		str << "; band " << lvl - hist_ig_pred::Half_Band_Size << " - " << lvl + hist_ig_pred::Half_Band_Size;
		str << "; " << dir_chars[static_cast<size_t>(pattern.x2())] << dir_chars[static_cast<size_t>(pattern.x())];
		str << "; freq: " << pattern.freq();
		str << std::endl;
	};

	used_rules << std::fixed << std::setprecision(1);
	unused_rules << std::fixed << std::setprecision(1);

	def_params << "const TParameters default_parameters = { ";
	def_params << mDt;

	const size_t pattern_count = static_cast<size_t>(hist_ig_pred::NPattern_Dir::count);
	for (auto band_idx = 0; band_idx < hist_ig_pred::Band_Count; band_idx++) {
		
		for (size_t x2 = 0; x2 < pattern_count; x2++) {
			for (size_t x = 0; x < pattern_count; x++) {
				hist_ig_pred::CPattern search_pattern{ static_cast<const size_t>(band_idx), static_cast<const hist_ig_pred::NPattern_Dir>(x2), static_cast<const hist_ig_pred::NPattern_Dir>(x) };

				def_params << ", ";

				auto iter = mPatterns.find(search_pattern);
				if (iter != mPatterns.end()) {
					def_params << hist_ig_pred::Level_To_Histogram_Index(iter->second.Level());					

					log_params(used_rules, used_counter, iter->second);

					x2x_freq[x2][x]++;
				}
				else {
					def_params << band_idx;	//stay in the same band if we know anything better

					log_params(unused_rules, unused_counter, search_pattern);
				}				

				def_params << ".0";
			}					
		}

		def_params << std::endl;
	}

	def_params << "};" << std::endl;

	dprintf("\nX2 X rules frequency\n");
	for (size_t x2 = 0; x2 < pattern_count; x2++) {
		for (size_t x = 0; x < pattern_count; x++) {
			dprintf(dir_chars[x2]);
			dprintf(dir_chars[x]);
			dprintf(": ");
			dprintf(x2x_freq[x2][x]);
			dprintf("\n");
		}
	}



	dprintf("\nDefault parameters:\n");
	dprintf(def_params.str());
	dprintf("\nUsed rules:\n");
	dprintf(used_rules.str());
	dprintf("\nUnused rules:\n");
	dprintf(unused_rules.str());
}



CHistogram_IG_Prediction_Signal::CHistogram_IG_Prediction_Signal(scgms::WTime_Segment segment) : 
	CCommon_Calculated_Signal(segment) {

	mIst = segment.Get_Signal(scgms::signal_IG);
	if (!mIst) throw std::exception{};
}

HRESULT IfaceCalling CHistogram_IG_Prediction_Signal::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	if (derivation_order != scgms::apxNo_Derivation) return E_NOTIMPL;

	const hist_ig_pred::TParameters &parameters = scgms::Convert_Parameters<hist_ig_pred::TParameters>(params, hist_ig_pred::default_parameters.vector.data());

	for (size_t i = 0; i < count; i++) {
		size_t band_idx;
		hist_ig_pred::NPattern_Dir x2, x;
		if (Classify(times[i] - mDt, band_idx, x2, x)) {
			const double raw_band = parameters.bands[static_cast<const size_t>(band_idx)][static_cast<const size_t>(x2)][static_cast<const size_t>(x)];
			levels[i] = hist_ig_pred::Histogram_Index_To_Level(
				static_cast<size_t>(round(raw_band)));
		}
		else
			levels[i] = std::numeric_limits<double>::quiet_NaN();
	}

	return S_OK;
}

HRESULT IfaceCalling CHistogram_IG_Prediction_Signal::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(hist_ig_pred::default_parameters.vector.data());
	return parameters->set(params, params + hist_ig_pred::default_parameters.vector.size());
}
