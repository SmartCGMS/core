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
				//case NInstruction::CSBOL_1:
				//	mRules[i] = std::make_unique<CCond_Set_1<NInstruction::CSBOL_1, NOutput_Variable::Bolus>>();
				//	break;
				case NInstruction::CSIBR_2:
					mRules[i] = std::make_unique<CCond_Set_2<NInstruction::CSIBR_2, NOutput_Variable::IBR>>();
					break;
				//case NInstruction::CSBOL_2:
				//	mRules[i] = std::make_unique<CCond_Set_2<NInstruction::CSBOL_2, NOutput_Variable::Bolus>>();
				//	break;
			}

			if (mRules[i])
			{
				if (!mRules[i]->Parse(input, cursor))
					return false;

				last_not_nop = i;
			}

			cursor += rule_length;
		}

		mRules.resize(last_not_nop + 1);

		Prune();

		return true;
	}

	void CContext::Prune()
	{
		// Here we prune rules, that conflict with themselves or other rules

		/*
		1) superceding rules
		  (X > 2) cmd; (X > 1) cmd --> (X > 1) cmd
		  (X < 1) cmd; (X < 2) cmd --> (X < 2) cmd
		  generalization
		  (X op a) cmd; (X op b) cmd, a op b cmd
		*/

		std::vector<std::string> plog1;

		// go through all rules and eliminate conflicting rules
		for (size_t i = 0; i < mRules.size(); i++)
		{
			if (!mRules[i])
				continue;

			try
			{
				flr_ge::CCond_Rule& crule1 = dynamic_cast<flr_ge::CCond_Rule&>(*mRules[i]);

				const flr_ge::CCond_Rule::TConditional& cond1 = crule1.Get_Cond_1();

				// conditional conflicts with itself --> delete
				if (cond1.Is_Self_Conflicting())
				{
					mRules[i].reset();
					continue;
				}

				try
				{
					const flr_ge::CCond_Rule::TConditional& cond2 = crule1.Get_Cond_2();

					// conditional conflicts with itself --> delete
					if (cond2.Is_Self_Conflicting())
					{
						mRules[i].reset();
						continue;
					}

					auto conflict = cond1.Is_Conflicting_With(*this, cond2, flr_ge::CCond_Rule::TConditional::NBool_Op::And);
					// conditional 1 conflicts with conditional 2
					if (conflict != flr_ge::CCond_Rule::TConditional::NConflict_Type::None)
					{
						switch (conflict)
						{
							// they are equivalent --> convert to a single conditional
							case flr_ge::CCond_Rule::TConditional::NConflict_Type::Equivalent:
								mRules[i]->Transcribe(*this, plog1);
								mRules[i] = crule1.Convert_To_Single(cond1);
								break;
							// the first supercedes the second --> convert to a single conditional
							case flr_ge::CCond_Rule::TConditional::NConflict_Type::Superceding_First:
								mRules[i]->Transcribe(*this, plog1);
								mRules[i] = crule1.Convert_To_Single(cond1);
								break;
							// the second supercedes the first --> convert to a single conditional
							case flr_ge::CCond_Rule::TConditional::NConflict_Type::Superceding_Second:
								mRules[i]->Transcribe(*this, plog1);
								mRules[i] = crule1.Convert_To_Single(cond2);
								break;
							// they are opposing --> delete
							case flr_ge::CCond_Rule::TConditional::NConflict_Type::Opposing:
								mRules[i]->Transcribe(*this, plog1);
								mRules[i].reset();
								break;
						}

						continue;
					}
				}
				catch (...)
				{
					// not a two-conditional rule
				}
			}
			catch (...)
			{
				// not a conditional rule
			}
		}

		bool conflict_found = false;
		bool inner_conflict_found = false;

		auto resolve_conflict = [this](flr_ge::CCond_Rule::TConditional::NConflict_Type conflict, flr_ge::CCond_Rule::TConditional& a, flr_ge::CCond_Rule::TConditional& b) {
			switch (conflict)
			{
				// they are equivalent --> delete first rule
				case flr_ge::CCond_Rule::TConditional::NConflict_Type::Equivalent:
					a.marked_for_delete = true;
					break;
				// the first supercedes the second --> delete the second rule
				case flr_ge::CCond_Rule::TConditional::NConflict_Type::Superceding_First:
					b.marked_for_delete = true;
					break;
				// the second supercedes the first --> delete the first rule
				case flr_ge::CCond_Rule::TConditional::NConflict_Type::Superceding_Second:
					a.marked_for_delete = true;
					break;
				// they are opposing --> delete first as they are basically superceding at this level
				case flr_ge::CCond_Rule::TConditional::NConflict_Type::Opposing:
					//a.marked_for_delete = true;
					//b.marked_for_delete = true;
					break;
			}
		};

		do
		{
			conflict_found = false;

			for (size_t i = 0; i < mRules.size(); i++)
			{
				if (!mRules[i])
					continue;

				try
				{
					flr_ge::CCond_Rule& crule1 = dynamic_cast<flr_ge::CCond_Rule&>(*mRules[i]);

					flr_ge::CCond_Rule::TConditional& cond1_1 = crule1.Get_Cond_1();

					inner_conflict_found = false;
					for (size_t j = i + 1; j < mRules.size(); j++)
					{
						if (!mRules[j])
							continue;

						try
						{
							flr_ge::CCond_Rule& crule2 = dynamic_cast<flr_ge::CCond_Rule&>(*mRules[j]);

							flr_ge::CCond_Rule::TConditional& cond2_1 = crule2.Get_Cond_1();

							// single cond (cond1_1) + single cond (cond2_1)
							if (crule1.Get_Cond_Count() == 1 && crule2.Get_Cond_Count() == 1)
							{
								auto conflict = cond1_1.Is_Conflicting_With(*this, cond2_1, flr_ge::CCond_Rule::TConditional::NBool_Op::Or);
								if (conflict != flr_ge::CCond_Rule::TConditional::NConflict_Type::None)
								{
									resolve_conflict(conflict, cond1_1, cond2_1);

									inner_conflict_found = true;
									break;
								}
							}
							else if (crule1.Get_Cond_Count() == 2 && crule2.Get_Cond_Count() == 2)
							{
								flr_ge::CCond_Rule::TConditional& cond1_2 = crule1.Get_Cond_2();
								flr_ge::CCond_Rule::TConditional& cond2_2 = crule2.Get_Cond_2();

								// (X OP a && Y op b) || (X OP a && Y !op b)
								// ???
							}
						}
						catch (...)
						{
							// not a conditional rule
						}
					}

					if (inner_conflict_found) {

						for (size_t j = 0; j < mRules.size(); j++)
						{
							if (!mRules[j])
								continue;

							try
							{
								flr_ge::CCond_Rule& crule1 = dynamic_cast<flr_ge::CCond_Rule&>(*mRules[j]);

								flr_ge::CCond_Rule::TConditional& cond1_1 = crule1.Get_Cond_1();

								try
								{
									flr_ge::CCond_Rule::TConditional& cond1_2 = crule1.Get_Cond_2();

									// double conditional
									if (cond1_1.marked_for_delete && cond1_2.marked_for_delete)
										mRules[j].reset();
									else if (cond1_1.marked_for_delete)
										mRules[j] = crule1.Convert_To_Single(cond1_2);
									else if (cond1_2.marked_for_delete)
										mRules[j] = crule1.Convert_To_Single(cond1_1);
								}
								catch (...)
								{
									// single conditional
									if (cond1_1.marked_for_delete)
										mRules[j].reset();
								}
							}
							catch (...)
							{
								// not a conditional
							}
						}

						conflict_found = true;
						continue;
					}

				}
				catch (...)
				{
					// not a conditional rule
				}
			}

		} while (conflict_found);

		// resize to last valid rule

		/*size_t last_not_nop = 0;

		for (size_t i = 0; i < mRules.size(); i++)
		{
			if (mRules[i])
				last_not_nop = i;
		}

		mRules.resize(last_not_nop + 1);*/
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

				avg_diff += (mPast_IG[b_idx] - mPast_IG[a_idx]) / (scgms::One_Minute * 5);
				avg += mPast_IG[b_idx];

				cnt++;
			}

			avg /= static_cast<double>(mPast_IG.size());
			avg_diff /= -static_cast<double>(mPast_IG.size());

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

		if (!Emit_IBR(mContext.Get_Output(flr_ge::NOutput_Variable::IBR), mCurrent_Time)) {
			return E_FAIL;
		}

		/*
		if (mContext.Get_Output(flr_ge::NOutput_Variable::Bolus) > 0)
		{
			if (!Emit_Bolus(mContext.Get_Output(flr_ge::NOutput_Variable::Bolus), mCurrent_Time))
				return E_FAIL;

			mLast_Model_Bolus = mCurrent_Time;

			mContext.Set_Output(flr_ge::NOutput_Variable::Bolus, 0);
		}
		*/
	}

	return S_OK;
}

void CFLR_GE_Model::Advance_Model(double time_advance_delta) {

	// for now, we assume the stepping is exactly 5 minutes - this causes the model to advance based on the current state of input variables and past state of intermediate variables

	//mContext.Set_Quantity(flr_ge::NQuantity::Minutes_From_Last_Bolus, (mCurrent_Time - mLast_Model_Bolus) / scgms::One_Minute);

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
