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

#include "../../../../common/iface/ApproxIface.h"
#include "../../../../common/utils/math_utils.h"
#include "../../../../common/utils/string_utils.h"
#include "../../../../common/utils/DebugHelper.h"
#include "../../../../common/utils/SimpleIni.h"
#include "../../../../common/rtl/FilesystemLib.h"

#undef min

CPattern_Prediction_Filter::CPattern_Prediction_Filter(scgms::IFilter *output)
	: scgms::CBase_Filter(output, pattern_prediction::filter_id), mUpdated_Levels(false) {
}

CPattern_Prediction_Filter::~CPattern_Prediction_Filter() {
	if (mUpdate_Parameters_File)
		Write_Parameters_File();
}

HRESULT CPattern_Prediction_Filter::Do_Execute(scgms::UDevice_Event event) {
	bool sent = false;

	auto handle_ig_level = [&event, &sent, this]() {

		const double dev_time = event.device_time();
		const uint64_t seg_id = event.segment_id();
		const double level = event.level();

		HRESULT rc = mOutput.Send(event);
		sent = Succeeded(rc);
		if (sent) {
			const double predicted_level = Update_And_Predict(seg_id, dev_time, level);

			if (std::isnormal(predicted_level)) {
				scgms::UDevice_Event prediction_event{ scgms::NDevice_Event_Code::Level };
				prediction_event.device_id() = pattern_prediction::filter_id;
				prediction_event.level() = predicted_level;
				prediction_event.device_time() = dev_time + mDt;
				prediction_event.signal_id() = pattern_prediction::signal_Pattern_Prediction;
				prediction_event.segment_id() = seg_id;
				rc = mOutput.Send(prediction_event);
			}
		}


		return rc;
	};

	auto handle_segment_stop = [&event, this]() {
		auto iter = mIst.find(event.segment_id());
		if (iter != mIst.end())
			mIst.erase(iter);

		if (mUpdate_Parameters_File)
			Write_Parameters_File();
	};


	HRESULT rc = E_UNEXPECTED;

	switch (event.event_code()) {
		case scgms::NDevice_Event_Code::Level:
			if (event.signal_id() == scgms::signal_IG)
				rc = handle_ig_level();
			break;

		case scgms::NDevice_Event_Code::Time_Segment_Stop:
			handle_segment_stop();
			break;

		case scgms::NDevice_Event_Code::Shut_Down:
			handle_segment_stop();
			break;

		default: break;
	}

	if (!sent) rc = mOutput.Send(event);

	return rc;

}


HRESULT CPattern_Prediction_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mDt = configuration.Read_Double(rsDt_Column, mDt);
	
	if (Is_Any_NaN(mDt))
		return E_INVALIDARG;
	
	mDo_Not_Learn = configuration.Read_Bool(rsDo_Not_Learn, mDo_Not_Learn);
	mParameters_File_Path = configuration.Read_File_Path(rsParameters_File);
	mUpdate_Parameters_File = !mParameters_File_Path.empty() &&
		                      !configuration.Read_Bool(rsDo_Not_Update_Parameters_File, !mUpdate_Parameters_File);

	if (!mParameters_File_Path.empty()) {
		HRESULT rc = Read_Parameters_File(error_description);
		if (Succeeded(rc))
			return rc;
	}

	return S_OK;
}

double CPattern_Prediction_Filter::Update_And_Predict(const uint64_t segment_id, const double current_time, const double current_level) {
	auto seg_iter = mIst.find(segment_id);
	if (seg_iter == mIst.end()) {
		scgms::SSignal new_ist = scgms::SSignal{ scgms::STime_Segment{}, scgms::signal_IG};
		if (new_ist) {
			auto inserted = mIst.insert(std::make_pair(segment_id, new_ist));
			seg_iter = inserted.first;
		}
		else
			return std::numeric_limits<double>::quiet_NaN();	//a failure with this segment
	}

	auto ist = seg_iter->second;

	//1st phase - learning
	Update_Learn(ist, current_time, current_level);

	//2nd phase - obtaining a learned result
	return Predict(ist, current_time);
}

void CPattern_Prediction_Filter::Update_Learn(scgms::SSignal& ist, const double current_time, const double current_ig_level) {
	if (Succeeded(ist->Update_Levels(&current_time, &current_ig_level, 1))) {
		auto [pattern, band_index, classified_ok] = Classify(ist, current_time - mDt);

		if (!mDo_Not_Learn) {

			if (classified_ok)
				mPatterns[static_cast<size_t>(pattern)][band_index].Update(current_ig_level);

			mUpdated_Levels = true;
		}
	}
}

std::tuple<CPattern_Prediction_Filter::NPattern, size_t, bool> CPattern_Prediction_Filter::Classify(scgms::SSignal& ist, const double current_time) {
	std::tuple<NPattern, size_t, bool> result{ NPattern::steady, Band_Count / 2, false };

	std::array<double, 3> levels;
	const std::array<double, 3> times = { current_time - 10.0 * scgms::One_Minute,
										  current_time - 5.0 * scgms::One_Minute,
										  current_time };
	if (ist->Get_Continuous_Levels(nullptr, times.data(), levels.data(), levels.size(), scgms::apxNo_Derivation) == S_OK) {
		if (!Is_Any_NaN(levels)) {
			std::get<NClassify::success>(result) = true;	//classified ok
			std::get<NClassify::band>(result) = Level_2_Band_Index(levels[2]);

			const double velocity = levels[2] - levels[0];

			if (velocity != 0.0) {
				const double d1 = fabs(levels[1] - levels[0]);
				const double d2 = fabs(levels[2] - levels[1]);
				const bool accelerating = d2 > d1;


				if (velocity > 0.0) {
					std::get<NClassify::pattern>(result) = accelerating && ((levels[1] > levels[0]) && (levels[2] > levels[1])) ?
						NPattern::accel : NPattern::up;
				}
				else if (velocity < 0.0) {
					std::get<NClassify::pattern>(result) = accelerating && ((levels[1] < levels[0]) && (levels[2] < levels[1])) ?
						NPattern::deccel : NPattern::down;
				}

			} //else already set to the steady pattern - see result declaration

		}
	}

	return result;
}

double CPattern_Prediction_Filter::Predict(scgms::SSignal& ist, const double current_time) {
	double predicted_level = std::numeric_limits<double>::quiet_NaN();
	auto [pattern, pattern_band_index, classified_ok] = Classify(ist, current_time);
	if (classified_ok) {

		const auto& patterns = mPatterns[static_cast<size_t>(pattern)];

		size_t band_counter = 0;
		for (size_t band_idx = 0; band_idx < Band_Count; band_idx++) {
			const auto& pattern = patterns[band_idx];

			if (pattern.Valid()) {
				const double dbl_band_idx = static_cast<double>(band_idx);
				mA(band_counter, 0) = dbl_band_idx * dbl_band_idx;
				mA(band_counter, 1) = dbl_band_idx;
				mA(band_counter, 2) = 1.0;
				mb(band_counter) = pattern.Level();

				band_counter++;
			}
		}


		if (band_counter > 3) {	//require an overfit, no >=			
			const Eigen::Vector3d coeff = mA.topRows(band_counter).fullPivHouseholderQr().solve(mb.topRows(band_counter));

			const double dlb_pattern_band_index = static_cast<double>(pattern_band_index);			
			predicted_level = coeff[0] * dlb_pattern_band_index * dlb_pattern_band_index +
				coeff[1] * dlb_pattern_band_index +
				coeff[2];

			if (!patterns[pattern_band_index].Valid()) {
				constexpr double min_level = Low_Threshold - Half_Band_Size;
				constexpr double max_level = High_Threshold + Half_Band_Size;

				predicted_level = std::min(predicted_level, max_level);
				predicted_level = std::max(predicted_level, min_level);
			}
		}		
	}

	return predicted_level;
}

size_t CPattern_Prediction_Filter::Level_2_Band_Index(const double level) {
	if (level >= High_Threshold) return Band_Count - 1;

	const double tmp = level - Low_Threshold;
	if (tmp < 0.0) return 0;

	return static_cast<size_t>(round(tmp * Inv_Band_Size));
}

HRESULT CPattern_Prediction_Filter::Read_Parameters_File(refcnt::Swstr_list error_description) {
	auto load_from_ini = [this](CSimpleIniW& ini) {

		std::list<CSimpleIniW::Entry> section_names;
		ini.GetAllSections(section_names);

		const std::wstring format = isPattern + std::wstring{L"%d"} + isBand + L"%d";
		for (auto& section_name : section_names) {
			int pattern_idx = std::numeric_limits<int>::max();
			int band_idx = std::numeric_limits<int>::max();

			if (swscanf_s(section_name.pItem, format.c_str(), &pattern_idx, &band_idx) == 2) {

				if ((pattern_idx < static_cast<int>(NPattern::count)) && (band_idx < static_cast<int>(Band_Count))) {

					auto& pattern = mPatterns[pattern_idx][band_idx];
					bool all_valid = true;

					auto read_dbl = [&ini, &section_name, &all_valid](const wchar_t* identifier)->double {
						bool valid = false;
						double val = str_2_dbl(ini.GetValue(section_name.pItem, identifier), valid);
					
						if (!valid || std::isnan(val))
							all_valid = false;
							
						return val;
					};

					TPattern_Prediction_Pattern_State pattern_state;
					pattern_state.count = read_dbl(iiCount);
					pattern_state.running_avg = read_dbl(iiAvg);
					pattern_state.running_median = read_dbl(iiMedian);
					pattern_state.running_variance_accumulator = read_dbl(iiVariance_Accumulator);
					pattern_state.running_stddev = read_dbl(iiStdDev);

					if (all_valid)
						pattern.Set_State(pattern_state);


				} //else, invalid record - let's ignore it
			}
		}


	};

	HRESULT rc = S_FALSE;
	try {
		std::ifstream configfile;
		configfile.open(mParameters_File_Path);

		if (configfile.is_open()) {
			std::vector<char> buf;
			buf.assign(std::istreambuf_iterator<char>(configfile), std::istreambuf_iterator<char>());
			// fix valgrind's "Conditional jump or move depends on uninitialised value(s)"
			// although we are sending proper length, SimpleIni probably reaches one byte further and reads uninitialized memory
			buf.push_back(0);
			
			CSimpleIniW ini;
			ini.LoadData(buf.data(), buf.size());
			load_from_ini(ini);

			configfile.close();
			rc = S_OK;
		}
		else {
			std::wstring desc = dsCannot_Open_File + mParameters_File_Path.wstring();
			error_description.push(desc);
		}


	}
	catch (const std::exception& ex) {
		// specific handling for all exceptions extending std::exception, except
		// std::runtime_error which is handled explicitly
		std::wstring error_desc = Widen_Char(ex.what());

		return E_FAIL;
	}
	catch (...) {
		rc = E_FAIL;
	}

	if (rc == S_OK) 
		mUpdated_Levels = false;	//if not S_OK, keep the current state
	return rc;	
}

void CPattern_Prediction_Filter::Write_Parameters_File() {
	if (!mUpdated_Levels) return;

	CSimpleIniW ini;

	for (size_t pattern_idx = 0; pattern_idx < static_cast<size_t>(NPattern::count); pattern_idx++) {
		for (size_t band_idx = 0; band_idx < Band_Count; band_idx++) {

			const auto& pattern = mPatterns[pattern_idx][band_idx];

			if (pattern.Valid()) {
				const auto pattern_state = pattern.Get_State();

				const std::wstring section_name = isPattern+std::to_wstring(pattern_idx) + isBand + std::to_wstring(band_idx);
				const wchar_t* section_name_ptr = section_name.c_str();

				auto set_val = [section_name_ptr, &ini](const wchar_t* ii, const double& val) {
					const auto str = dbl_2_wstr(val);
					ini.SetValue(section_name_ptr, ii, str.c_str());
				};
                
				set_val(iiCount, pattern_state.count);
				set_val(iiAvg, pattern_state.running_avg);
				set_val(iiMedian, pattern_state.running_median);
				set_val(iiVariance_Accumulator, pattern_state.running_variance_accumulator);
				set_val(iiStdDev, pattern_state.running_stddev);

				//diagnostic
				//ini.SetLongValue(section_name_ptr, L"Predicted_Band", Level_2_Band_Index(pattern_state.running_median));
			}
		}
	}

	std::string content;
	ini.Save(content);
	
	std::ofstream config_file(mParameters_File_Path, std::ofstream::binary);
	if (config_file.is_open()) {
		config_file << content;
		config_file.close();
	}
}
