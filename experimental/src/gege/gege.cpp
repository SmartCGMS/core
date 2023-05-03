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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "gege.h"
#include "../descriptor.h"

#include "../../../../common/rtl/SolverLib.h"

#include "../../../../common/utils/string_utils.h"

/*

<S> ::= <rule> | <rule> <S>

<rule> ::= <cond> <action> | <action>

<cond> ::= IF (<bool_expression> AND <bool_expression>) | IF (<bool_expression>)

<bool_expression> ::= <quantity> <operator> <quantity> | <quantity> <operator> <constant>

<action> ::= SET_IBR(<constant>) | SET_BOLUS(<constant>)

------

<constant> 

*/

namespace gege
{
	void CContext::Evaluate()
	{
		mStart_Node->Evaluate(*this);
	}

	bool CContext::Parse(const std::vector<double>& genome)
	{
		size_t idx = 0;
		return mStart_Node->Parse(genome, idx);
	}

	void CContext::Transcribe(std::vector<std::string>& target)
	{
		mStart_Node->Transcribe(target);
	}
}

CGEGE_Model::CGEGE_Model(scgms::IModel_Parameter_Vector * parameters, scgms::IFilter * output)
	: CBase_Filter(output),
	mParameters(scgms::Convert_Parameters<gege::TParameters>(parameters, gege::default_parameters.vector))
{
	std::vector<double> pars;
	pars.assign(mParameters.vector, mParameters.vector + gege::param_count);

	mContext.Parse(pars);
}

HRESULT CGEGE_Model::Do_Execute(scgms::UDevice_Event event) {

	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		if (event.signal_id() == scgms::signal_IG)
		{
			mContext.Set_State_Variable(gege::NQuantity::IG, event.level());

			// 0 slope for the first iteration
			if (!mHas_IG)
			{
				mHas_IG = true;
				std::fill(mPast_IG.begin(), mPast_IG.end(), event.level());
				std::fill(mPast_10_IG.begin(), mPast_10_IG.end(), event.level());
			}
			else
			{
				mPast_IG[mPast_IG_Cursor] = event.level();
				mPast_IG[mPast_IG_10_Cursor] = event.level();
			}

			// calculate average slope (this will probably be a subject of future work)
			size_t cnt = 0;
			double avg_diff = 0;
			double avg = mPast_IG[mPast_IG_Cursor];
			for (size_t i = 0; i < mPast_IG.size() - 1; i++)
			{
				const size_t a_idx = (mPast_IG_Cursor + i) % mPast_IG.size();
				const size_t b_idx = (mPast_IG_Cursor + i + 1) % mPast_IG.size();

				if (cnt == 0)
					avg_diff = (mPast_IG[b_idx] - mPast_IG[a_idx]) / (scgms::One_Minute * 5);
				else
					avg_diff = (avg_diff * static_cast<double>(cnt) + (mPast_IG[b_idx] - mPast_IG[a_idx]) / (scgms::One_Minute * 5)) / static_cast<double>(cnt + 1);

				avg += (avg * static_cast<double>(cnt) + mPast_IG[b_idx]) / static_cast<double>(cnt + 1);

				cnt++;
			}

			double avg_10 = mPast_10_IG[mPast_IG_10_Cursor];
			cnt = 0;
			for (size_t i = 5; i < mPast_10_IG.size(); i++)
			{
				const size_t a_idx = (mPast_IG_10_Cursor + i) % mPast_10_IG.size();

				avg_10 += (avg_10 * static_cast<double>(cnt) + mPast_IG[a_idx]) / static_cast<double>(cnt + 1);
				cnt++;
			}


			mContext.Set_State_Variable(gege::NQuantity::IG_Slope, avg_diff);
			mContext.Set_State_Variable(gege::NQuantity::IG_Average_Last_0_5, avg);
			mContext.Set_State_Variable(gege::NQuantity::IG_Average_Last_6_10, avg_10);

			mPast_IG_Cursor = (mPast_IG_Cursor + 1) % mPast_IG.size();
		}
		else if (event.signal_id() == scgms::signal_Carb_Intake || event.signal_id() == scgms::signal_Carb_Rescue)
		{
			mLast_Meal = event.device_time();
			mLast_Meal_Size = event.level();
		}
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down)
	{
		std::vector<std::string> target;
		mContext.Transcribe(target);

		std::string so;
		for (auto& s : target)
			so += s + " | ";

		Emit_Info(scgms::NDevice_Event_Code::Information, Widen_String(so), mSegment_id);
	}

	return mOutput.Send(event);
}

HRESULT CGEGE_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list & error_description) {

	return S_OK;
}

HRESULT IfaceCalling CGEGE_Model::Initialize(const double new_current_time, const uint64_t segment_id) {

	mCurrent_Time = new_current_time;
	mSegment_id = segment_id;

	return S_OK;
}

HRESULT IfaceCalling CGEGE_Model::Step(const double time_advance_delta) {

	if (time_advance_delta > 0.0) {
		// really advance the model here
		Advance_Model(time_advance_delta);

		// advance the timestamp
		mCurrent_Time += time_advance_delta;
	}

	if (!Emit_IBR(mContext.Get_Output_IBR(), mCurrent_Time + scgms::One_Minute * 5)) {
		return E_FAIL;
	}

	if (mContext.Get_Output_Bolus() > 0)
	{
		//if ((mLast_Model_Bolus + scgms::One_Minute * 180.0) < mCurrent_Time)
		{
			if (!Emit_Bolus(mContext.Get_Output_Bolus(), mCurrent_Time + scgms::One_Minute * 5))
				return E_FAIL;

			mLast_Model_Bolus = mCurrent_Time;
		}

		mContext.Set_Output_Bolus(0);
	}

	return S_OK;
}

void CGEGE_Model::Advance_Model(double time_advance_delta) {

	// for now, we assume the stepping is exactly 5 minutes - this causes the model to advance based on the current state of input variables and past state of intermediate variables

	mContext.Evaluate();

}

bool CGEGE_Model::Emit_IBR(double level, double time)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
	evt.signal_id() = gege::ibr_id;
	evt.device_id() = gege::model_id;
	evt.device_time() = time;
	evt.segment_id() = mSegment_id;
	evt.level() = level;
	return Succeeded(mOutput.Send(evt));
}

bool CGEGE_Model::Emit_Bolus(double level, double time)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
	evt.signal_id() = gege::bolus_id;
	evt.device_id() = gege::model_id;
	evt.device_time() = time;
	evt.segment_id() = mSegment_id;
	evt.level() = level;
	return Succeeded(mOutput.Send(evt));
}
