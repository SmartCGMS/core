#include "aim_ge.h"
#include "../descriptor.h"

#include "../../../../common/rtl/SolverLib.h"

#include "../../../../common/utils/string_utils.h"
#include "../../../../common/utils/math_utils.h"
#include "../../../../common/utils/DebugHelper.h"

namespace aim_ge
{
	template<typename T, typename... Args>
	std::unique_ptr<CRule> instantiate(double codon, Args&&... args)
	{
		std::unique_ptr<CRule> rule = std::make_unique<T>(std::forward<Args>(args)...);
		rule->Parse(codon, 0);
		return rule;
	}

	bool CContext::Parse(const std::vector<double>& input)
	{
		if (Is_Any_NaN(input))
			return false;

		// OLD: <e_gluc> <op> <func>(<e_ins> <op> <e_cho>)

		/*mRoot_Rule = instantiate<COperator_Rule>(input[5],
			instantiate<CQuantity_Term_Rule>(input[4], NInstruction::E_GLUC),
			instantiate<CFunction_Rule>(input[3],
				instantiate<COperator_Rule>(input[2],
					instantiate<CQuantity_Term_Rule>(input[0], NInstruction::E_INS),
					instantiate<CQuantity_Term_Rule>(input[1], NInstruction::E_CHO)
				)
			)
		);*/

		// NEW: <e_gluc> <op> ( <number> * <func>(<e_ins>) <op> <number> * <func>(<e_cho>) )
		// NEW: <gluc(t)> <op> ( <number> * <func>(<e_ins>) <op> <number> * <func>(<e_cho>) )

		/*mRoot_Rule = instantiate<COperator_Rule>(input[0],
			instantiate<CQuantity_Term_Rule>(input[1], NInstruction::E_GLUC),
			instantiate<COperator_Rule>(input[2],
				instantiate<COperator_Rule>(0.0,
					instantiate<CNumber_Rule>(input[3]), 
					instantiate<CFunction_Rule>(input[4],
						instantiate<CQuantity_Term_Rule>(input[5], NInstruction::E_INS)
				), NOperator::MUL),
				instantiate<COperator_Rule>(0.0,
					instantiate<CNumber_Rule>(input[6]),
					instantiate<CFunction_Rule>(input[7],
						instantiate<CQuantity_Term_Rule>(input[8], NInstruction::E_CHO)
				), NOperator::MUL)
			)
		);*/

		mRoot_Rule = instantiate<COperator_Rule>(input[0],
			instantiate<CInput_Quantity_Rule>(0, NInput_Quantity::GLUC),
			instantiate<COperator_Rule>(input[2],
				instantiate<COperator_Rule>(0.0,
					instantiate<CNumber_Rule>(input[3]),
					instantiate<CFunction_Rule>(input[4],
						instantiate<CQuantity_Term_Rule>(input[5], NInstruction::E_INS)
						), NOperator::MUL),
				instantiate<COperator_Rule>(0.0,
					instantiate<CNumber_Rule>(input[6]),
					instantiate<CFunction_Rule>(input[7],
						instantiate<CQuantity_Term_Rule>(input[8], NInstruction::E_CHO)
						), NOperator::MUL)
				)
			);

		return true;
	}
}

CAIM_GE_Model::CAIM_GE_Model(scgms::IModel_Parameter_Vector * parameters, scgms::IFilter * output)
	: CBase_Filter(output),
	mParameters(scgms::Convert_Parameters<aim_ge::TParameters>(parameters, aim_ge::default_parameters.vector)),
	mContext(*this)
{
	std::vector<double> pars;
	pars.assign(mParameters.codons, mParameters.codons + aim_ge::codons_count);

	if (!mContext.Parse(pars))
		throw std::runtime_error{ "Cannot parse rules" };
}

double CAIM_GE_Model::Get_Input_Quantity(aim_ge::NInput_Quantity q, size_t timeOffset)
{
	// offset is an index from the end
	switch (q)
	{
		case aim_ge::NInput_Quantity::GLUC:
			return mPastIG.size() > timeOffset ? *(mPastIG.rbegin() + timeOffset) : mParameters.IG_0;
		case aim_ge::NInput_Quantity::INS:
			return Calc_IOB_At(mCurrent_Time + static_cast<double>(timeOffset) * 5.0 * scgms::One_Minute);
		case aim_ge::NInput_Quantity::CHO:
			return Calc_COB_At(mCurrent_Time + static_cast<double>(timeOffset) * 5.0 * scgms::One_Minute);
		default:
			return 0;
	}

	return 0;
}

HRESULT CAIM_GE_Model::Do_Execute(scgms::UDevice_Event event)
{

	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		if (event.signal_id() == scgms::signal_IG)
		{
			mPastIG.push_back(event.level());
		}
		else if (event.signal_id() == scgms::signal_Carb_Intake)
		{
			mStored_Carbs.push_back({
				event.device_time(),
				static_cast<double>(event.device_time() + 5.0_hr),
				event.level()
				});
		}
		else if (event.signal_id() == scgms::signal_Requested_Insulin_Bolus || event.signal_id() == scgms::signal_Delivered_Insulin_Bolus)
		{
			mStored_Insulin.push_back({
				event.device_time(),
				static_cast<double>(event.device_time() + 5.0_hr),
				event.level() // U -> mU
				});
		}
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down)
	{
		std::vector<std::string> target;
		mContext.Transcribe(target);

		std::string so;
		for (auto& s : target)
			so += s + " ";

		Emit_Info(scgms::NDevice_Event_Code::Information, Widen_String(so), mSegment_id);
	}

	return mOutput.Send(event);
}

HRESULT CAIM_GE_Model::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list & error_description)
{
	return S_OK;
}

HRESULT IfaceCalling CAIM_GE_Model::Initialize(const double new_current_time, const uint64_t segment_id)
{
	mCurrent_Time = new_current_time;
	mSegment_id = segment_id;

	return S_OK;
}

HRESULT IfaceCalling CAIM_GE_Model::Step(const double time_advance_delta)
{
	if (time_advance_delta > 0.0)
	{
		// really advance the model here
		Advance_Model(time_advance_delta);

		// advance the timestamp
		mCurrent_Time += time_advance_delta;

		double ig = mContext.Get_Output(aim_ge::NOutput_Variable::IG);
		// we want only normal values (no infinities, NaNs and so)
		if (std::fpclassify(ig) != FP_NORMAL)
			ig = 0;

		if (!Emit_IG(ig, mCurrent_Time + aim_ge::Prediction_Horizon))
			return E_FAIL;

		auto cleanup = [this](auto& cont) {
			for (auto itr = cont.begin(); itr != cont.end(); )
			{
				if (itr->time_end + aim_ge::Prediction_Horizon < mCurrent_Time)
					itr = cont.erase(itr);
				else
					++itr;
			}
		};

		cleanup(mStored_Insulin);
		cleanup(mStored_Carbs);

		// IOB
		/*{
			scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
			evt.signal_id() = scgms::signal_Virtual[60];
			evt.device_id() = aim_ge::model_id;
			evt.device_time() = mCurrent_Time;
			evt.segment_id() = mSegment_id;
			evt.level() = Calc_COB_At(mCurrent_Time);
			mOutput.Send(evt);
		}

		// COB
		{
			scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
			evt.signal_id() = scgms::signal_Virtual[61];
			evt.device_id() = aim_ge::model_id;
			evt.device_time() = mCurrent_Time;
			evt.segment_id() = mSegment_id;
			evt.level() = Calc_IOB_At(mCurrent_Time);
			mOutput.Send(evt);
		}*/
	}
	else if (time_advance_delta == 0.0 && mPastIG.empty())
	{
		for (double t = mCurrent_Time; t < mCurrent_Time + aim_ge::Prediction_Horizon; t += 5.0_min)
		Emit_IG(mParameters.IG_0, t);
	}

	return S_OK;
}

void CAIM_GE_Model::Advance_Model(double time_advance_delta)
{
	// for now, we assume the stepping is exactly 5 minutes - this causes the model to advance based on the current state of input variables and past state of intermediate variables

	mContext.Evaluate();
}

bool CAIM_GE_Model::Emit_IG(double level, double time)
{
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
	evt.signal_id() = aim_ge::ig_id;
	evt.device_id() = aim_ge::model_id;
	evt.device_time() = time;
	evt.segment_id() = mSegment_id;
	evt.level() = level;
	return Succeeded(mOutput.Send(evt));
}

double CAIM_GE_Model::Calc_IOB_At(const double time)
{
	double iob_acc = 0.0;

	constexpr double t_max = 55;
	constexpr double Vi = 0.12;
	constexpr double ke = 0.138;

	double S1 = 0, S2 = 0, I = 0;
	double T = 0;

	if (!mStored_Insulin.empty())
	{

		for (auto& r : mStored_Insulin)
		{
			if (T == 0)
			{
				T = r.time_start;
				S1 += r.value;
			}
			else
			{
				while (T < r.time_start && T < time)
				{
					T += scgms::One_Minute;

					I += (S2 / (Vi * t_max)) - (ke * I);
					S2 += (S1 - S2) / t_max;
					S1 += 0 - S1 / t_max;
				}

				S1 += r.value;
			}
		}

		while (T < time)
		{
			T += scgms::One_Minute;

			I += (S2 / (Vi * t_max)) - (ke * I);
			S2 += (S1 - S2) / t_max;
			S1 += 0 - S1 / t_max;
		}

	}

	iob_acc = I;

	return iob_acc;
}

double CAIM_GE_Model::Calc_COB_At(const double time)
{
	double cob_acc = 0.0;

	//C(t) = (Dg * Ag * t * e^(-t/t_max)) / t_max^2

	constexpr double t_max = 40; //min
	constexpr double Ag = 0.8;

	constexpr double GlucMolar = 180.16; // glucose molar weight (g/mol)
	constexpr double gCHO_to_mmol = (1000 / GlucMolar); // 1000 -> g to mg, 180.16 - molar weight of glucose; g -> mmol
	constexpr double mmol_to_gCHO = 1.0 / gCHO_to_mmol;

	for (auto& r : mStored_Carbs)
	{
		if (time >= r.time_start && time < r.time_end)
		{
			const double t = (time - r.time_start) / scgms::One_Minute; // device_time (rat time) to minutes

			const double Dg = gCHO_to_mmol * r.value;

			cob_acc += (Dg * Ag * t * std::exp(-t / t_max)) / std::pow(t_max, 2.0);
			//cob_acc += Dg * Ag - (Dg * Ag * (t_max - std::exp(-t / t_max) * (t_max + t)) / t_max);
		}
	}

	return cob_acc * mmol_to_gCHO;
}
