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

#include "pattern_prediction.h"
#include "../descriptor.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/utils/math_utils.h"
#include "../../../../common/utils/DebugHelper.h"

#include <numeric>
#include <sstream>
#include <iomanip>

namespace pattern_prediction {
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
		mFailures.setZero();
	}


	void CPattern::Update(const double future_level) {
		const size_t band_idx = Level_To_Histogram_Index(future_level);
		mHistogram(band_idx) += 1.0;

		//updated, let's verify succes of this prediction
		const size_t predicted_band_idx = Level_To_Histogram_Index(Level());
		if (band_idx != predicted_band_idx) {
			mFailures(predicted_band_idx) += 1.0;	//increase negative rating for this level
		}
	}

	double CPattern::Level() const {

		
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

		auto find_centroid_rating = [&]() {
			THistogram prob = mHistogram;
			
			for (auto i = 0; i < prob.cols(); i++) {
				const double rating = mHistogram(i) > 0.0 ?
					mHistogram(i) / (mFailures(i) + mHistogram(i))
					: 0.0;

				prob(i) *= rating;
			}

			const double mx = prob.maxCoeff();
			if (mx > 0.0) {
				prob /= mx;

				const double area = prob.sum();
				double moments = 0.0;
				for (auto i = 0; i < prob.cols(); i++) {					
					moments += prob(i) *static_cast<double>(i);
				}

				const size_t result_idx = static_cast<size_t>(std::round(moments / area));

				return Histogram_Index_To_Level(result_idx);
			}
			else
				return std::numeric_limits<double>::quiet_NaN();
		};
				
		return find_centroid_rating();
	}

	bool CPattern::operator< (const CPattern &other) const {
		if (mBand_Idx != other.mBand_Idx) return mBand_Idx < other.mBand_Idx;
		if (mX2 != other.mX2) return mX2 < other.mX2;
		return mX < other.mX;
	}

}

pattern_prediction::NPattern_Dir CPattern_Classification::dbl_2_pat(const double x) const {
	if (x < 0.0) return pattern_prediction::NPattern_Dir::negative;
	else if (x > 0.0) return pattern_prediction::NPattern_Dir::positive;

	return pattern_prediction::NPattern_Dir::zero;
}


bool CPattern_Classification::Classify(const double current_time, size_t &band_idx, pattern_prediction::NPattern_Dir &x2, pattern_prediction::NPattern_Dir &x) const {
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

			//actually, we will keep this experiment as it performs better than the polynomial approach
			//x2_raw = 1.0 in both cases is surprising, but does not seem to be an error after all
			//it seems that we only the basic direction and bool acceleration
#define Dexperiment
#ifdef Dexperiment		
			const double x_raw = levels[2] - levels[0];
			double x2_raw = 0.0;

			if (x_raw != 0.0) {
				const double d1 = fabs(levels[1] - levels[0]);
				const double d2 = fabs(levels[2] - levels[1]);
				const bool acc = d2 > d1;
				if (acc) {
					if (x_raw > 0.0) {
						if ((levels[1] > levels[0]) && (levels[2] > levels[1])) x2_raw = 1.0;
					}

					if (x_raw < 0.0) {
						if ((levels[1] < levels[0]) && (levels[2] < levels[1])) x2_raw = 1.0;
					}
				}
			}
#undef Dexperiment
#else
			const double x2_raw = 0.5*(levels[2] + levels[0]) - levels[1];
			const double x_raw = levels[1] - levels[0] - x2_raw;
#endif
			band_idx = pattern_prediction::Level_To_Histogram_Index(levels[2]);
			x2 = dbl_2_pat(x2_raw);
			x = dbl_2_pat(x_raw);
			result = true;
		}
	}

	return result;
}


CPattern_Prediction_Filter::CPattern_Prediction_Filter(scgms::IFilter *output) 
	: scgms::CBase_Filter(output, pattern_prediction::filter_id) {
}

CPattern_Prediction_Filter::~CPattern_Prediction_Filter() {
	if (mDump_Params)
		Dump_Params();
}

HRESULT CPattern_Prediction_Filter::Do_Execute(scgms::UDevice_Event event) {
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
				pred.device_id() = pattern_prediction::filter_id;
				pred.level() = pred_level;
				pred.device_time() = dev_time + mDt;
				pred.signal_id() = pattern_prediction::signal_Pattern_Prediction;
				pred.segment_id() = seg_id;
				rc = Send(pred);
			}
		}

		return rc;
	}
	else
		return Send(event);	
}

HRESULT CPattern_Prediction_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mDt = configuration.Read_Double(rsDt_Column, mDt);

	mIst = scgms::SSignal{ scgms::STime_Segment{}, scgms::signal_IG };
	return S_OK;
}

double CPattern_Prediction_Filter::Update_And_Predict(const double current_time, const double ig_level) {
	double result = std::numeric_limits<double>::quiet_NaN();
 
	//1st phase - learning
	if (SUCCEEDED(mIst->Update_Levels(&current_time, &ig_level, 1))) {
		size_t band_idx;
		pattern_prediction::NPattern_Dir x2, x;
		if (Classify(current_time - mDt, band_idx, x2, x)) {			
			pattern_prediction::CPattern pattern{ band_idx, x2, x };

			auto iter = mPatterns.find(pattern);				
			if (iter == mPatterns.end()) {
				auto x = mPatterns.insert(std::pair< pattern_prediction::CPattern, pattern_prediction::CPattern>(  pattern, std::move(pattern)));					
				iter = x.first;
			}
							
			iter->second.Update(ig_level);			
		}
	}


	//2nd phase - obtaining a learned result
	{
		size_t band_idx;
		pattern_prediction::NPattern_Dir x2, x;
		if (Classify(current_time, band_idx, x2, x)) {
			pattern_prediction::CPattern pattern{ band_idx, x2, x };

			auto iter = mPatterns.find(pattern);
			if (iter != mPatterns.end()) 
				result = iter->second.Level();						
		}
	}

	return result;
}

void CPattern_Prediction_Filter::Dump_Params() {
	std::stringstream def_params, used_rules, unused_rules;
	size_t unused_counter = 0, used_counter = 0;
	const char *dir_chars = "v-^";

	std::stringstream lb_params, effective_params, ub_params;

	std::array<std::array<size_t, 3>, 3> x2x_freq;
	memset(&x2x_freq, 0, sizeof(x2x_freq));

	auto log_params = [&](std::stringstream &str, size_t &counter, pattern_prediction::CPattern &pattern) {
		counter++;
		str << "counter " << counter;
		const double lvl = pattern_prediction::Histogram_Index_To_Level(pattern.band_idx());
		str << "; band index: " << pattern.band_idx();
		str << "; band " << lvl - pattern_prediction::Half_Band_Size << " - " << lvl + pattern_prediction::Half_Band_Size;
		str << "; " << dir_chars[static_cast<size_t>(pattern.x2())] << dir_chars[static_cast<size_t>(pattern.x())];
		str << "; freq: " << pattern.freq();
		str << "; stddev: " << pattern.stdev();
		str << std::endl;
	};

	used_rules << std::fixed << std::setprecision(1);
	unused_rules << std::fixed << std::setprecision(1);

	def_params << "const TParameters default_parameters = { ";
	def_params << mDt;

	lb_params << mDt << " ";
	effective_params << mDt << " ";
	ub_params << mDt << " ";

	const size_t pattern_count = static_cast<size_t>(pattern_prediction::NPattern_Dir::count);
	for (auto band_idx = 0; band_idx < pattern_prediction::Band_Count; band_idx++) {
		
		for (size_t x2 = 0; x2 < pattern_count; x2++) {
			for (size_t x = 0; x < pattern_count; x++) {
				pattern_prediction::CPattern search_pattern{ static_cast<const size_t>(band_idx), static_cast<const pattern_prediction::NPattern_Dir>(x2), static_cast<const pattern_prediction::NPattern_Dir>(x) };

				def_params << ", ";				

				auto iter = mPatterns.find(search_pattern);
				if (iter != mPatterns.end()) {
					def_params << pattern_prediction::Level_To_Histogram_Index(iter->second.Level());					

					log_params(used_rules, used_counter, iter->second);

					x2x_freq[x2][x]++;

					lb_params << 0 << " ";
					effective_params << band_idx << " ";
					ub_params << pattern_prediction::Band_Count - 1 << " ";
				}
				else {
					def_params << band_idx;	//stay in the same band if we know anything better

					lb_params << band_idx << " ";	//unused => do not optimize
					effective_params << band_idx << " ";
					ub_params << band_idx << " ";

					log_params(unused_rules, unused_counter, search_pattern);
				}				

				def_params << ".0";
			}					
		}

		def_params << std::endl;
	}

	def_params << "};" << std::endl;

	dprintf("\nX2 X rules frequency:\n");
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

	lb_params << " " << effective_params.str() << " " << ub_params.str();
	dprintf("\nIni parameters:\n");
	dprintf("Model_Bounds = ");
	dprintf(lb_params.str());
	dprintf("\n");
}



CPattern_Prediction_Signal::CPattern_Prediction_Signal(scgms::WTime_Segment segment) : 
	CCommon_Calculated_Signal(segment) {

	mIst = segment.Get_Signal(scgms::signal_IG);
	if (!mIst) throw std::exception{};
}

HRESULT IfaceCalling CPattern_Prediction_Signal::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	if (derivation_order != scgms::apxNo_Derivation) return E_NOTIMPL;

	const pattern_prediction::TParameters &parameters = scgms::Convert_Parameters<pattern_prediction::TParameters>(params, pattern_prediction::default_parameters.vector.data());

	for (size_t i = 0; i < count; i++) {
		size_t band_idx;
		pattern_prediction::NPattern_Dir x2, x;
		if (Classify(times[i] - mDt, band_idx, x2, x)) {
			const double raw_band = parameters.bands[static_cast<const size_t>(band_idx)][static_cast<const size_t>(x2)][static_cast<const size_t>(x)];
			levels[i] = pattern_prediction::Histogram_Index_To_Level(
				static_cast<size_t>(round(raw_band)));			
		}
		else
			levels[i] = std::numeric_limits<double>::quiet_NaN();
	}

	return S_OK;
}

HRESULT IfaceCalling CPattern_Prediction_Signal::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(pattern_prediction::default_parameters.vector.data());
	return parameters->set(params, params + pattern_prediction::default_parameters.vector.size());
}
