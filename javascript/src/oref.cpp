/* Examples and Documentation for
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
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "oref.h"
#include "oref_settings.h"
#include "descriptor.h"

#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/FilesystemLib.h"
#include "../../../common/utils/string_utils.h"
#include "../../../common/rtl/rattime.h"

#include <cmath>
#include <algorithm>
#include <numeric>

#include "v8_support.h"

#include <thread>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iostream>

namespace external
{
	// oref0 explicitly expects to have these signals as inputs

	constexpr GUID signal_Insulin_Activity_Exponential = { 0x19974ff5, 0x9eb1, 0x4a99, { 0xb7, 0xf2, 0xdc, 0x9b, 0xfa, 0xe, 0x31, 0x5e } };
	constexpr GUID signal_IOB_Exponential = { 0x238d2353, 0x6d37, 0x402c, { 0xaf, 0x39, 0x6c, 0x55, 0x52, 0xa7, 0x7e, 0x1f } };
	constexpr GUID signal_COB_Bilinear = { 0xe29a9d38, 0x551e, 0x4f3f, { 0xa9, 0x1d, 0x1f, 0x14, 0xd9, 0x34, 0x67, 0xe3 } };
}

COref_Discrete_Model::COref_Discrete_Model(scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output)
	: CBase_Filter(output),
	  mParameters(scgms::Convert_Parameters<oref_model::TParameters>(parameters, oref_model::default_parameters)) {

	mState.current_basal_rate = 0.0;
	mState.calculated_smb = 0.0;

	mSegment = refcnt::make_shared_reference_ext<scgms::STime_Segment, scgms::ITime_Segment>(new scgms::CTime_Segment(), false);
	mIG = mSegment.Get_Signal(scgms::signal_IG);
	mCarbs = mSegment.Get_Signal(scgms::signal_Carb_Intake);
	mBoluses = mSegment.Get_Signal(scgms::signal_Delivered_Insulin_Bolus);
	mDeliveredInsulin = mSegment.Get_Signal(scgms::signal_Delivered_Insulin_Total);

	mCOB = mSegment.Get_Signal(external::signal_COB_Bilinear);
	mIOBCalc = mSegment.Get_Signal(external::signal_IOB_Exponential);
	mInsActCalc = mSegment.Get_Signal(external::signal_Insulin_Activity_Exponential);

	mOref_Path_Base = "C:/Programy/oref0/"; // TODO: move to some kind of manifest
}

HRESULT COref_Discrete_Model::Do_Execute(scgms::UDevice_Event event) {

	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		const double time = event.device_time();
		const double level = event.level();

		if (event.signal_id() == scgms::signal_IG)
		{
			if (mIG->Update_Levels(&time, &level, 1) != S_OK)
				return E_FAIL;
		}
		else if (event.signal_id() == scgms::signal_Delivered_Insulin_Bolus)
		{
			if (mBoluses->Update_Levels(&time, &level, 1) != S_OK)
				return E_FAIL;
		}
		else if (event.signal_id() == scgms::signal_Carb_Intake || event.signal_id() == scgms::signal_Carb_Rescue)
		{
			if (mCarbs->Update_Levels(&time, &level, 1) != S_OK)
				return E_FAIL;
		}
		else if (event.signal_id() == scgms::signal_Delivered_Insulin_Total)
		{
			if (mDeliveredInsulin->Update_Levels(&time, &level, 1) != S_OK)
				return E_FAIL;
		}
	}

	// just forward the event through the filter chain
	return Send(event);
}

HRESULT COref_Discrete_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {

	if (!mJSEnv.Initialize()) {
		return E_FAIL;
	}

	mJSEnv.Locked_Run_Program_From_File(mOref_Path_Base + "lib/round-basal.js");
	mJSEnv.Locked_Run_Program_From_File(mOref_Path_Base + "lib/basal-set-temp.js");
	mJSEnv.Locked_Run_Program_From_File(mOref_Path_Base + "lib/determine-basal/determine-basal.js");

	return S_OK;
}

double COref_Discrete_Model::Calculate_Avg_Blood(double hist_start, double hist_end, double step) const
{
	size_t levelCount = static_cast<size_t>((hist_end - hist_start) / step);

	if (levelCount == 0) {
		return std::numeric_limits<double>::quiet_NaN();
	}

	double delta = 0.0;
	std::vector<double> deltaTimes(levelCount);
	std::vector<double> deltaLevels(levelCount);
	std::generate(deltaTimes.begin(), deltaTimes.end(), [tmn = hist_start - oref_model::delta_history_sample_step]() mutable {
		return (tmn += oref_model::delta_history_sample_step);
	});

	if (mIG->Get_Continuous_Levels(nullptr, deltaTimes.data(), deltaLevels.data(), levelCount, scgms::apxNo_Derivation) != S_OK) {
		return std::numeric_limits<double>::quiet_NaN();
	}

	delta = std::accumulate(deltaLevels.begin(), deltaLevels.end(), delta) / scgms::mgdl_2_mmoll; // to mg/dl

	return (delta / static_cast<double>(levelCount));
}

HRESULT IfaceCalling COref_Discrete_Model::Initialize(double new_current_time, uint64_t segment_id) {

	mCurrent_Time = new_current_time;
	mSegment_Id = segment_id;

	return S_OK;
}

bool COref_Discrete_Model::Call_Oref(double time_advance_delta)
{
	std::vector<double> futureTimes;
	std::vector<double> carbTimes, carbValues, bolusTimes, bolusValues;
	size_t cnt;

	mCarbs->Get_Discrete_Bounds(nullptr, nullptr, &cnt);
	carbTimes.resize(cnt);
	carbValues.resize(cnt);

	mCarbs->Get_Discrete_Levels(carbTimes.data(), carbValues.data(), cnt, &cnt);
	carbTimes.resize(cnt);
	carbValues.resize(cnt);

	mBoluses->Get_Discrete_Bounds(nullptr, nullptr, &cnt);
	bolusTimes.resize(cnt);
	bolusValues.resize(cnt);

	mBoluses->Get_Discrete_Levels(bolusTimes.data(), bolusValues.data(), cnt, &cnt);
	bolusTimes.resize(cnt);
	bolusValues.resize(cnt);

	double curBG;
	mIG->Get_Continuous_Levels(nullptr, &mCurrent_Time, &curBG, 1, scgms::apxNo_Derivation);

	const double nowTime = mCurrent_Time + time_advance_delta;
	const double glucose = curBG / scgms::mgdl_2_mmoll; // oref0 takes inputs in mg/dl

	mLastState = mState;

	// assume failure until the value could be calculated
	mState.current_basal_rate = 0.0;
	mState.calculated_smb = 0.0;

	COref_Instance_Data data;

	// calculate deltas
	data.mDelta = glucose - Calculate_Avg_Blood(nowTime - oref_model::delta_history_start, nowTime - oref_model::delta_history_end, oref_model::delta_history_sample_step);
	data.mShortAvgDelta = glucose - Calculate_Avg_Blood(nowTime - oref_model::short_avgdelta_history_start, nowTime - oref_model::short_avgdelta_history_end, oref_model::short_avgdelta_history_sample_step);
	data.mLongAvgDelta = glucose - Calculate_Avg_Blood(nowTime - oref_model::long_avgdelta_history_start, nowTime - oref_model::long_avgdelta_history_end, oref_model::long_avgdelta_history_sample_step);

	if (std::isnan(data.mDelta) || std::isnan(data.mShortAvgDelta) || std::isnan(data.mLongAvgDelta)) {
		return false;
	}

	futureTimes.resize(12);
	data.mCurIOB.resize(futureTimes.size());
	data.mCurInsulinActivity.resize(futureTimes.size());

	std::fill(data.mCurIOB.begin(), data.mCurIOB.end(), std::numeric_limits<double>::quiet_NaN());
	std::fill(data.mCurInsulinActivity.begin(), data.mCurInsulinActivity.end(), std::numeric_limits<double>::quiet_NaN());

	std::generate(futureTimes.begin(), futureTimes.end(), [gtime = nowTime - scgms::One_Minute * 5]() mutable {
		return gtime += oref_model::assumed_stepping;
	});

	double cob = 0;
	if (mCOB->Get_Continuous_Levels(nullptr, &nowTime, &cob, 1, scgms::apxNo_Derivation) != S_OK) {
		return false;
	}
	if (mIOBCalc->Get_Continuous_Levels(nullptr, futureTimes.data(), data.mCurIOB.data(), data.mCurIOB.size(), scgms::apxNo_Derivation) != S_OK) {
		return false;
	}
	if (mInsActCalc->Get_Continuous_Levels(nullptr, futureTimes.data(), data.mCurInsulinActivity.data(), data.mCurInsulinActivity.size(), scgms::apxNo_Derivation) != S_OK) {
		return false;
	}

	data.mCurTime = Rat_Time_To_Unix_Time(nowTime);
	data.mLastTime = Rat_Time_To_Unix_Time(mCurrent_Time);
	data.mGlucose = glucose;

	// carb times are ordered; rbegin points to last dosed carbs
	if (!std::isnan(cob) && cob > 0 && carbTimes.size() > 0 && !std::isnan(*carbValues.rbegin()) && *carbValues.rbegin() > 0) {
		data.mLastCarbTime = Rat_Time_To_Unix_Time(*carbTimes.rbegin());
		data.mLastCarbValue = *carbValues.rbegin();
		data.mCurCOB = cob;
	} else {
		data.mLastCarbTime = 0;
		data.mLastCarbValue = 0;
		data.mCurCOB = 0;
	}

	// bolus times are ordered; rbegin points to last dosed bolus
	if (bolusTimes.size() > 0 && !std::isnan(*bolusValues.rbegin()) && *bolusValues.rbegin() > 0) {
		data.mLastBolusTime = Rat_Time_To_Unix_Time(*bolusTimes.rbegin());
		data.mLastBolusValue = *bolusValues.rbegin();
	} else {
		data.mLastBolusTime = 0;
		data.mLastBolusValue = 0;
	}

	data.mResultRate = mLastState.current_basal_rate;
	data.mResultDuration = 30;

	std::string program = oref_utils::Prepare_Base_Program(data, mParameters);

	std::vector<double> result;
	bool res = mJSEnv.Run_And_Fetch(program, result);

	if (res && !result.empty())
	{
		mState.current_basal_rate = std::isnan(result[0]) ? 0 : result[0];
		mState.calculated_smb = std::isnan(result[1]) ? 0 : result[1];
	}

	return true;
}

HRESULT IfaceCalling COref_Discrete_Model::Step(const double time_advance_delta) {

	// send exceptions (as error log messages) if any
	const auto& exceptions = mJSEnv.Get_Exceptions_List();
	if (!exceptions.empty()) {

		for (const auto& exc : exceptions) {
			if (!Emit_Error(exc, mCurrent_Time))
				return E_FAIL;
		}

		mJSEnv.Flush_Exceptions();
	}

	if (time_advance_delta > 0.0) {

		Call_Oref(time_advance_delta);

		// advance the timestamp
		mCurrent_Time += time_advance_delta;

		if (!std::isnan(mState.calculated_smb) && mState.calculated_smb > 0) {
			if (!Emit_Level(oref_model::oref0_smb_signal_id, mCurrent_Time, mState.calculated_smb)) {
				return E_FAIL;
			}
		}
	}

	if (!Emit_Level(oref_model::oref0_basal_rate_signal_id, mCurrent_Time, mState.current_basal_rate)) {
		return E_FAIL;
	}

	return S_OK;
}

bool COref_Discrete_Model::Emit_Level(const GUID& signal_id, double time, double level)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
	evt.signal_id() = signal_id;
	evt.segment_id() = mSegment_Id;
	evt.device_id() = oref_model::model_id;
	evt.device_time() = time;
	evt.level() = level;
	return SUCCEEDED(Send(evt));
}

bool COref_Discrete_Model::Emit_Error(const std::wstring& str, double time)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Error };
	evt.signal_id() = Invalid_GUID;
	evt.segment_id() = mSegment_Id;
	evt.device_id() = oref_model::model_id;
	evt.device_time() = time;
	evt.info.set(str.c_str());
	return SUCCEEDED(Send(evt));
}
