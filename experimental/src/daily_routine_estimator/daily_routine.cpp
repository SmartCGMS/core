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

#include "daily_routine.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/iface/DeviceIface.h"
#include "../../../../common/utils/math_utils.h"

#include "../../../../common/utils/DebugHelper.h"

using namespace scgms::literals;

CDaily_Routine_Model::CDaily_Routine_Model(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output)
	: CBase_Filter(output), mParameters(scgms::Convert_Parameters<daily_routine::TParameters>(parameters, daily_routine::default_parameters.vector))
{
	//
}

HRESULT CDaily_Routine_Model::Do_Execute(scgms::UDevice_Event event)
{
	return mOutput.Send(event);
}

HRESULT CDaily_Routine_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description)
{
	return S_OK;
}

HRESULT IfaceCalling CDaily_Routine_Model::Initialize(const double current_time, const uint64_t segment_id)
{
	mCurrent_Time = current_time;
	mSegment_Id = segment_id;

	return S_OK;
}

HRESULT IfaceCalling CDaily_Routine_Model::Step(const double time_advance_delta)
{
	if (time_advance_delta < 0)
		return E_ILLEGAL_STATE_CHANGE;

	const double beforeFraction = mCurrent_Time - std::floor(mCurrent_Time);

	mCurrent_Time += time_advance_delta;

	const double curDay = std::floor(mCurrent_Time);
	const double currentFraction = mCurrent_Time - std::floor(mCurrent_Time);

	for (size_t i = 0; i < daily_routine::daily_meal_count; i++) {

		// do not dose carbs, if we are below some threshold - this is to allow for proper function of optimization algorithms, which are penalized for
		// unmatched carb dosage to actually dosed levels
		if (mParameters.carb_values[i] < 1.0)
			continue;

		if ((beforeFraction < currentFraction && beforeFraction < mParameters.carb_times[i] && currentFraction >= mParameters.carb_times[i])
			|| (beforeFraction > currentFraction && beforeFraction > mParameters.carb_times[i] && currentFraction >= mParameters.carb_times[i])
			) {
			Emit_Estimated_Carb(curDay + mParameters.carb_times[i], mParameters.carb_values[i]);
		}
	}

	return S_OK;
}

bool CDaily_Routine_Model::Emit_Estimated_Carb(double time, double value) {

	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
	evt.device_id() = daily_routine::model_id;
	evt.device_time() = time;
	evt.level() = value;
	evt.segment_id() = mSegment_Id;
	evt.signal_id() = daily_routine::est_carbs_id;

	return Succeeded(mOutput.Send(evt));
}
