#include "flr_ge.h"
#include "../descriptor.h"

#include "../../../../common/rtl/SolverLib.h"

#include "../../../../common/utils/string_utils.h"
#include "../../../../common/utils/DebugHelper.h"

namespace flr_ge
{
	bool CContext::Parse(const std::vector<double>& input, size_t rule_count, size_t rule_length)
	{
		mRules.clear();
		mRules.resize(rule_count);

		size_t cursor = 0;
		size_t last_not_nop = 0;

		for (size_t i = 0; i < rule_count; i++)
		{
			auto opcode = classify<NInstruction>(input[cursor]);

			switch (opcode)
			{
				case NInstruction::NOP:
					break;
				case NInstruction::CSIBR_1:
					mRules[i] = std::make_unique<CCond_Set_1<NInstruction::CSIBR_1, NOutput_Variable::IBR>>();
					break;
				case NInstruction::CSBOL_1:
					mRules[i] = std::make_unique<CCond_Set_1<NInstruction::CSBOL_1, NOutput_Variable::Bolus>>();
					break;
				case NInstruction::CSIBR_2:
					mRules[i] = std::make_unique<CCond_Set_2<NInstruction::CSIBR_2, NOutput_Variable::IBR>>();
					break;
				case NInstruction::CSBOL_2:
					mRules[i] = std::make_unique<CCond_Set_2<NInstruction::CSBOL_2, NOutput_Variable::Bolus>>();
					break;
			}

			if (mRules[i])
			{
				last_not_nop = i;

				if (!mRules[i]->Parse(input, cursor))
					return false;
			}

			cursor += rule_length;
		}

		mRules.resize(last_not_nop + 1);

		return true;
	}
}

CFLR_GE_Model::CFLR_GE_Model(scgms::IModel_Parameter_Vector * parameters, scgms::IFilter * output)
	: CBase_Filter(output),
	mParameters(scgms::Convert_Parameters<flr_ge::TParameters>(parameters, flr_ge::default_parameters.vector))
{
	std::vector<double> pars;
	pars.assign(mParameters.vector, mParameters.vector + flr_ge::param_count);

	mContext.Set_Constants(pars, flr_ge::rules_count * flr_ge::rule_size, flr_ge::constants_count);

	if (!mContext.Parse(pars, flr_ge::rules_count, flr_ge::rule_size))
		throw std::runtime_error{ "Cannot parse rules" };
}

HRESULT CFLR_GE_Model::Do_Execute(scgms::UDevice_Event event) {

	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		if (event.signal_id() == scgms::signal_IG)
		{
			mContext.Set_Quantity(flr_ge::NQuantity::IG, event.level());

			// 0 slope for the first iteration
			if (!mHas_IG)
			{
				mHas_IG = true;
				std::fill(mPast_IG.begin(), mPast_IG.end(), event.level());
			}
			else
			{
				mPast_IG[mPast_IG_Cursor] = event.level();
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

			mContext.Set_Quantity(flr_ge::NQuantity::IG_Avg5, avg);
			mContext.Set_Quantity(flr_ge::NQuantity::IG_Avg5_Slope, avg_diff);

			mPast_IG_Cursor = (mPast_IG_Cursor + 1) % mPast_IG.size();
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

HRESULT CFLR_GE_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list & error_description) {

	return S_OK;
}

HRESULT IfaceCalling CFLR_GE_Model::Initialize(const double new_current_time, const uint64_t segment_id) {

	mCurrent_Time = new_current_time;
	mSegment_id = segment_id;

	return S_OK;
}

HRESULT IfaceCalling CFLR_GE_Model::Step(const double time_advance_delta) {

	if (time_advance_delta > 0.0) {
		// really advance the model here
		Advance_Model(time_advance_delta);

		// advance the timestamp
		mCurrent_Time += time_advance_delta;
	}

	if (!Emit_IBR(mContext.Get_Output(flr_ge::NOutput_Variable::IBR), mCurrent_Time)) {
		return E_FAIL;
	}

	if (mContext.Get_Output(flr_ge::NOutput_Variable::Bolus) > 0)
	{

		if (!Emit_Bolus(mContext.Get_Output(flr_ge::NOutput_Variable::Bolus), mCurrent_Time))
			return E_FAIL;

		mLast_Model_Bolus = mCurrent_Time;

		mContext.Set_Output(flr_ge::NOutput_Variable::Bolus, 0);
	}

	return S_OK;
}

void CFLR_GE_Model::Advance_Model(double time_advance_delta) {

	// for now, we assume the stepping is exactly 5 minutes - this causes the model to advance based on the current state of input variables and past state of intermediate variables

	mContext.Set_Quantity(flr_ge::NQuantity::Minutes_From_Last_Bolus, (mCurrent_Time - mLast_Model_Bolus) / scgms::One_Minute);

	mContext.Evaluate();
}

bool CFLR_GE_Model::Emit_IBR(double level, double time)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
	evt.signal_id() = flr_ge::ibr_id;
	evt.device_id() = flr_ge::model_id;
	evt.device_time() = time;
	evt.segment_id() = mSegment_id;
	evt.level() = level;
	return Succeeded(mOutput.Send(evt));
}

bool CFLR_GE_Model::Emit_Bolus(double level, double time)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
	evt.signal_id() = flr_ge::bolus_id;
	evt.device_id() = flr_ge::model_id;
	evt.device_time() = time;
	evt.segment_id() = mSegment_id;
	evt.level() = level;
	return Succeeded(mOutput.Send(evt));
}
