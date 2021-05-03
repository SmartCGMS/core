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
	
	if (Is_Any_NaN(mDt)) {
		error_description.push(L"Prediction horizon must be positive number.");

		return E_INVALIDARG;
	}
	
	mDo_Not_Learn = configuration.Read_Bool(rsDo_Not_Learn, mDo_Not_Learn);
	mParameters_File_Path = configuration.Read_File_Path(rsParameters_File);
	mUpdate_Parameters_File = !mParameters_File_Path.empty() &&
		                      !configuration.Read_Bool(rsDo_Not_Update_Parameters_File, !mUpdate_Parameters_File);


	//parameters loading must go as the last!
	mUse_Config_Parameters = configuration.Read_Bool(rsUse_Config_parameters, false);

	if (!mUse_Config_Parameters) {
		//loading parameters from the external .ini file
		if (!mParameters_File_Path.empty()) {
			const HRESULT rc = Read_Parameters_File(error_description);
			if (Succeeded(rc))
				return rc;
		}
	} else {
		//using parameters supplied by scgms, e.g.; by a solver
		return Read_Parameters_From_Config(configuration, error_description);
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
				mPatterns[static_cast<size_t>(pattern)][band_index].push(current_ig_level);

			mUpdated_Levels = true;
		}
	}
}

std::tuple<pattern_prediction::NPattern, size_t, bool> CPattern_Prediction_Filter::Classify(scgms::SSignal& ist, const double current_time) {
	std::tuple<pattern_prediction::NPattern, size_t, bool> result{ pattern_prediction::NPattern::steady, pattern_prediction::Band_Count / 2, false };

	std::array<double, 4> levels;
	const std::array<double, 4> times = { current_time - 15.0 * scgms::One_Minute,
										  current_time - 10.0 * scgms::One_Minute,
										  current_time - 5.0 * scgms::One_Minute,
										  current_time };
	if (ist->Get_Continuous_Levels(nullptr, times.data(), levels.data(), levels.size(), scgms::apxNo_Derivation) == S_OK) {
		if (!Is_Any_NaN(levels)) {
			
			//acceleration and band-index are good on non-smoothed data
			const bool acc = std::fabs(levels[3] - levels[2]) > std::fabs(levels[2] - levels[1]);

			std::get<NClassify::success>(result) = true;	//classified ok
			std::get<NClassify::band>(result) = Level_2_Band_Index(levels[3]);

			//pattern recognition works best on smoothed data
			const double l10 = 0.25 * levels[0] + 0.5 * levels[1] + 0.25 * levels[2];
			const double l05 = 0.25 * levels[1] + 0.5 * levels[2] + 0.25 * levels[3];			
			const double l00 = 0.25 * levels[1] + 0.25 * levels[2] + 0.5 * levels[3];					

			levels[0] = l10;
			levels[1] = l05;
			levels[2] = l00;

			const bool agb = levels[0] > levels[1];
			const bool alb = !agb;

			const bool bgc = levels[1] > levels[2];
			const bool blc = !bgc;

			const bool agc = levels[0] > levels[2];
			const bool alc = !agc;
			
			if (agb && bgc && acc) std::get<NClassify::pattern>(result) = pattern_prediction::NPattern::deccel;
			else if (agb && bgc &&(!acc)) std::get<NClassify::pattern>(result) = pattern_prediction::NPattern::down;
			else if (agb && blc && agc) std::get<NClassify::pattern>(result) = pattern_prediction::NPattern::convex_slow;
			else if (agb && blc && alc) std::get<NClassify::pattern>(result) = pattern_prediction::NPattern::convex_fast;
			else if (alb && bgc && agc) std::get<NClassify::pattern>(result) = pattern_prediction::NPattern::concave_fast;
			else if (alb && bgc && alc) std::get<NClassify::pattern>(result) = pattern_prediction::NPattern::concave_slow;
			else if (alb && blc && (!acc)) std::get<NClassify::pattern>(result) = pattern_prediction::NPattern::up;
			else if (alb && blc && acc) std::get<NClassify::pattern>(result) = pattern_prediction::NPattern::accel;
			else std::get<NClassify::pattern>(result) = pattern_prediction::NPattern::steady;
		}
	}

	return result;
}

double CPattern_Prediction_Filter::Predict(scgms::SSignal& ist, const double current_time) {
	double predicted_level = std::numeric_limits<double>::quiet_NaN();
	auto [pattern, pattern_band_index, classified_ok] = Classify(ist, current_time);
	if (classified_ok) {

		auto& patterns = mPatterns[static_cast<size_t>(pattern)];
		auto& pattern = patterns[pattern_band_index];
		if (pattern)
			predicted_level = pattern.predict();
	}

	return predicted_level;
}

size_t CPattern_Prediction_Filter::Level_2_Band_Index(const double level) {
	if (level >= pattern_prediction::High_Threshold) return pattern_prediction::Band_Count - 1;

	const double tmp = level - pattern_prediction::Low_Threshold;
	if (tmp < 0.0) return 0;

	return static_cast<size_t>(round(tmp * pattern_prediction::Inv_Band_Size));
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

				if ((pattern_idx < static_cast<int>(pattern_prediction::NPattern::count)) && (band_idx < static_cast<int>(pattern_prediction::Band_Count))) {

					auto& pattern = mPatterns[pattern_idx][band_idx];
					bool all_valid = true;

					std::wstring state = ini.GetValue(section_name.pItem, iiState);
					pattern.State_from_String(state);

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

HRESULT CPattern_Prediction_Filter::Read_Parameters_From_Config(scgms::SFilter_Configuration configuration, refcnt::Swstr_list error_description) {
	std::vector<double> lower, def, upper;

	if (configuration.Read_Parameters(rsParameters, lower, def, upper)) {
		if (def.size() != pattern_prediction::model_param_count) {
			error_description.push(L"Corrupted pattern-prediction configuration parameters!");
			return E_INVALIDARG;
		}


		size_t def_idx = 0;

		for (size_t pattern_idx = 0; pattern_idx < static_cast<size_t>(pattern_prediction::NPattern::count); pattern_idx++) {
			for (size_t band_idx = 0; band_idx < pattern_prediction::Band_Count; band_idx++) {
				auto& pattern = mPatterns[pattern_idx][band_idx];

				pattern.Set_State(def[def_idx]);

				def_idx++;
			}
		}
	}
	else
		return S_FALSE;	//indicate empty parameters

	return S_OK;
}

void CPattern_Prediction_Filter::Write_Parameters_File() {
	if (!mUpdated_Levels && !mUse_Config_Parameters) return;

	CSimpleIniW ini;

	for (size_t pattern_idx = 0; pattern_idx < static_cast<size_t>(pattern_prediction::NPattern::count); pattern_idx++) {
		for (size_t band_idx = 0; band_idx < pattern_prediction::Band_Count; band_idx++) {

			const auto& pattern = mPatterns[pattern_idx][band_idx];

			if (pattern) {
				const auto pattern_state = pattern.State_To_String();

				const std::wstring section_name = isPattern+std::to_wstring(pattern_idx) + isBand + std::to_wstring(band_idx);
				const wchar_t* section_name_ptr = section_name.c_str();
				ini.SetValue(section_name_ptr, iiState, pattern_state.c_str());

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
