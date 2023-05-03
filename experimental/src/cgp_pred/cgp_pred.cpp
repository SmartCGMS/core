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

#include "cgp_pred.h"
#include "../../../../common/iface/DeviceIface.h"
#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/utils/string_utils.h"

#include "../../../../common/utils/DebugHelper.h"
#include "../../../../common/utils/math_utils.h"

#include <array>

using namespace scgms::literals;

/*

INPUTS:
G(t-0), G(t-5), ..., G(t-120) => 25
I(t+0), I(t+5), ..., I(t+120) => 25
C(t+0), C(t+5), ..., C(t+120) => 25

Total = 75

*/

template<typename TType, typename... T>
static inline constexpr std::array<TType, sizeof...(T)> make_array(T &&...t)
{
	return { std::forward<T>(t)... };
}

const auto FuncArray = make_array<std::function<double(double, double)>>(
	[](double a, double b) { return a + b; },
	[](double a, double b) { return a - b; },
	[](double a, double b) { return a * b; },
	//[](double a, double b) { return a / std::sqrt(1 + b * b); },
	[](double a, double b) { return std::tanh(a); },
	//[](double a, double b) { return std::sin(a); },
	//[](double a, double b) { return std::sqrt(std::abs(a)); },
	[](double a, double b) { return std::log(1 + std::abs(a)); },
	//[](double a, double b) { return std::exp(a); },
	[](double a, double b) { return (a + b) / 2.0; }
	//[](double a, double b) { return a + (b - a); }
);

const auto FuncTranscribeArray = make_array<std::function<std::string(const std::string&, const std::string&)>>(
	[](const std::string& a, const std::string& b) { return a + "+" + b; },
	[](const std::string& a, const std::string& b) { return a + "-" + b; },
	[](const std::string& a, const std::string& b) { return a + "*" + b; },
	//[](const std::string& a, const std::string& b) { return "aq(" +a + ", " + b + ")"; },
	[](const std::string& a, const std::string& b) { return "tanh(" + a + ")"; },
	//[](const std::string& a, const std::string& b) { return "sin(" + a + ")"; },
	//[](const std::string& a, const std::string& b) { return "psqrt(" + a + ")"; },
	[](const std::string& a, const std::string& b) { return "plog(" + a + ")"; },
	//[](const std::string& a, const std::string& b) { return "exp(" + a + ")"; },
	[](const std::string& a, const std::string& b) { return "avg(" + a + ", " + b + ")"; }
	//[](const std::string& a, const std::string& b) { return "xpl(" + a + ", " + b + ")"; }
);

constexpr size_t GlucoseWindowStart = 36;	// start
constexpr size_t GlucoseWindowEnd = 0;// 288 - 13;	// one value further to comply with "iterable"
constexpr size_t TotalGlucoseValues = GlucoseWindowStart - GlucoseWindowEnd;

constexpr size_t TotalInsulinValues = 13;
constexpr size_t TotalCarbValues = 13;
constexpr double ValueSpacing = 5.0_min;

constexpr size_t TotalInputs = TotalGlucoseValues + TotalInsulinValues + TotalCarbValues + cgp_pred::TotalConstantCount;

static_assert(GlucoseWindowStart > GlucoseWindowEnd, "Glucose windows are probably switched or too small");
static_assert(FuncArray.size() == FuncTranscribeArray.size(), "Function table and transcribe table sizes does not match");

CCGP_Prediction::CCGP_Prediction(scgms::IModel_Parameter_Vector* parameters, scgms::IFilter* output)
	: CBase_Filter(output), mParameters(scgms::Convert_Parameters<cgp_pred::TParameters>(parameters, cgp_pred::default_parameters.data())) {
	

	Fill_Evaluation_Vector();
}

constexpr inline size_t Rescale(const double in, const size_t numClasses)
{
	return static_cast<size_t>(in * static_cast<double>(numClasses) - 0.00001);
}

void CCGP_Prediction::Fill_Evaluation_Vector()
{
	const size_t size = TotalInputs + cgp_pred::MatrixWidth * cgp_pred::MatrixLength;

	mOutputs.resize(size);
	mToEvaluate.resize(size);
	std::fill(mToEvaluate.begin(), mToEvaluate.end(), false);

	for (size_t p = 0; p < cgp_pred::NumOfForecastValues; ++p)
	{
		const size_t res = Rescale(mParameters.outputs[p], cgp_pred::MatrixWidth * cgp_pred::MatrixLength);
		mToEvaluate[TotalInputs + res] = true;
		mConverted_Params.outputs[p] = res;
	}

	for (size_t p = size - 1; p >= TotalInputs; --p)
	{
		if (mToEvaluate[p])
		{
			const size_t ChosenFunction = Rescale(mParameters.nodes[p - TotalInputs].function, FuncArray.size());
			mConverted_Params.nodes[p - TotalInputs].function = ChosenFunction;

			// forbid the first parameter to be a constant
			const size_t res1 = Rescale(mParameters.nodes[p - TotalInputs].operands[0], p - cgp_pred::TotalConstantCount - 1) + cgp_pred::TotalConstantCount;
			mToEvaluate[res1] = true;
			mConverted_Params.nodes[p - TotalInputs].operands[0] = res1;

			const size_t res2 = Rescale(mParameters.nodes[p - TotalInputs].operands[1], p - 1);
			mToEvaluate[res2] = true;
			mConverted_Params.nodes[p - TotalInputs].operands[1] = res1;
		}
	}

	for (size_t i = 0; i < cgp_pred::TotalConstantCount; i++)
		mConverted_Params.constants[i] = (mParameters.constants[i] - 0.5) * 20.0;

	for (size_t i = 0; i < cgp_pred::TotalConstantCount; i++)
		mOutputs[i] = mConverted_Params.constants[i];
}

HRESULT CCGP_Prediction::Do_Execute(scgms::UDevice_Event event) {
	if (event.event_code() == scgms::NDevice_Event_Code::Level)
	{
		if (event.signal_id() == scgms::signal_IG)
		{
			mPastIG.push_back(event.level());
			if (mPastIG.size() > GlucoseWindowStart)
				mPastIG.erase(mPastIG.begin());
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
				event.level()
			});
		}
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down)
	{
		auto transcribeInput = [this](const size_t input) -> std::string {

			if (input < cgp_pred::TotalConstantCount + TotalGlucoseValues)
			{
				const size_t valOffset = GlucoseWindowStart - (input - cgp_pred::TotalConstantCount);
				return "G(t-" + std::to_string(valOffset * 5) + ")";
			}
			else if (input < cgp_pred::TotalConstantCount + TotalGlucoseValues + TotalInsulinValues)
			{
				const size_t valIdx = input - (cgp_pred::TotalConstantCount + TotalGlucoseValues);
				return "I(t+" + std::to_string(valIdx * 5) + ")";
			}
			else if (input < cgp_pred::TotalConstantCount + TotalGlucoseValues + TotalInsulinValues + TotalCarbValues)
			{
				const size_t valIdx = input - (cgp_pred::TotalConstantCount + TotalGlucoseValues + TotalInsulinValues);
				return "C(t+" + std::to_string(valIdx * 5) + ")";
			}
			else if (input < cgp_pred::TotalConstantCount)
				return std::to_string(mConverted_Params.constants[input]);
			else
				return "0";
		};

		std::function<std::string(size_t)> resolve = [&resolve, &transcribeInput, this](size_t p) -> std::string {

			const size_t ChosenFunction = mConverted_Params.nodes[p - TotalInputs].function;//Rescale(mParameters.nodes[p - TotalInputs].function, FuncArray.size());
			const size_t res1 = mConverted_Params.nodes[p - TotalInputs].operands[0];//Rescale(mParameters.nodes[p - TotalInputs].operands[0], p);
			const size_t res2 = mConverted_Params.nodes[p - TotalInputs].operands[1];//Rescale(mParameters.nodes[p - TotalInputs].operands[1], p);

			std::string lhs = res1 < TotalInputs ? transcribeInput(res1) : resolve(res1);
			std::string rhs = res2 < TotalInputs ? transcribeInput(res2) : resolve(res2);

			return "[" + std::to_string(p) + "]" + FuncTranscribeArray[ChosenFunction](lhs, rhs);
		};

		// transcription for output index 0 (for now only this one)
		std::string transcription = resolve(TotalInputs + Rescale(mParameters.outputs[cgp_pred::NumOfForecastValues - 1], cgp_pred::MatrixWidth * cgp_pred::MatrixLength));
		std::wstring wstr = Widen_String(transcription);

		scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Information };
		evt.signal_id() = cgp_pred::calculated_signal_ids[0];
		evt.device_id() = cgp_pred::model_id;
		evt.device_time() = mCurrent_Time + 120.0_min;
		evt.segment_id() = mSegment_Id;
		evt.info.set(wstr.c_str());
		mOutput.Send(evt);
	}

	return mOutput.Send(event);
}

HRESULT CCGP_Prediction::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	return S_OK;
}

HRESULT IfaceCalling CCGP_Prediction::Initialize(const double current_time, const uint64_t segment_id) {

	mCurrent_Time = current_time;
	mSegment_Id = segment_id;

	return S_OK;
}

HRESULT IfaceCalling CCGP_Prediction::Step(const double time_advance_delta) {

	if (time_advance_delta > 0)
	{
		mCurrent_Time += time_advance_delta;

		if (mPastIG.size() < GlucoseWindowStart)
			return S_OK;

		const int size = TotalInputs + cgp_pred::MatrixWidth * cgp_pred::MatrixLength;

		std::vector<double> times(std::max(TotalInsulinValues, TotalCarbValues));
		std::vector<double> vals(std::max(TotalInsulinValues, TotalCarbValues));

		std::copy(mPastIG.begin() + (mPastIG.size() - GlucoseWindowStart), mPastIG.begin() + (mPastIG.size() - GlucoseWindowEnd), mOutputs.begin() + cgp_pred::TotalConstantCount);
		
		std::generate(times.begin(), times.end(), [p = mCurrent_Time - ValueSpacing]() mutable { return (p += ValueSpacing); });

		Calc_IOB_At(times, vals);
		std::copy(vals.begin(), vals.end(), mOutputs.begin() + cgp_pred::TotalConstantCount + TotalGlucoseValues);

		Calc_COB_At(times, vals);
		std::copy(vals.begin(), vals.end(), mOutputs.begin() + cgp_pred::TotalConstantCount + TotalGlucoseValues + TotalInsulinValues);

		for (size_t p = TotalInputs; p < size; ++p)
		{
			if (mToEvaluate[p])
			{
				// decode the individual - function/operator and two operands
				const size_t ChosenFunction = mConverted_Params.nodes[p - TotalInputs].function;//Rescale(mParameters.nodes[p - TotalInputs].function, FuncArray.size());
				const size_t res1 = mConverted_Params.nodes[p - TotalInputs].operands[0];//Rescale(mParameters.nodes[p - TotalInputs].operands[0], p);
				const size_t res2 = mConverted_Params.nodes[p - TotalInputs].operands[1];//Rescale(mParameters.nodes[p - TotalInputs].operands[1], p);

				mOutputs[p] = FuncArray[ChosenFunction](mOutputs[res1], mOutputs[res2]);
			}
		}

		//for (size_t i = 0; i < cgp_pred::NumOfForecastValues; i++)
		size_t i = cgp_pred::NumOfForecastValues - 1;
		{
			// output just the last value for now - it is an output of some hidden unit
			const size_t r = mConverted_Params.outputs[i]; //Rescale(mParameters.outputs[i], cgp_pred::MatrixWidth * cgp_pred::MatrixLength);

			double g120 = mOutputs[TotalInputs + r];
			if (std::fpclassify(g120) != FP_NORMAL)
				g120 = 0;

			scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
			evt.signal_id() = cgp_pred::calculated_signal_ids[i];
			evt.device_id() = cgp_pred::model_id;
			evt.device_time() = mCurrent_Time + static_cast<double>(i+1)*30.0_min;
			evt.segment_id() = mSegment_Id;
			evt.level() = g120;
			mOutput.Send(evt);
		}

		auto cleanup = [this](auto& cont) {
			for (auto itr = cont.begin(); itr != cont.end(); )
			{
				if (itr->time_end + 5.0_min < mCurrent_Time)
					itr = cont.erase(itr);
				else
					++itr;
			}
		};

		cleanup(mStored_Insulin);
		cleanup(mStored_Carbs);
	}

	return S_OK;
}

double CCGP_Prediction::Calc_IOB_At(const std::vector<double>& time, std::vector<double>& result)
{
	double iob_acc = 0.0;

	constexpr double t_max = 55;
	constexpr double Vi = 0.12;
	constexpr double ke = 0.138;

	double S1 = 0, S2 = 0, I = 0;
	double T = 0;

	std::fill(result.begin(), result.end(), std::numeric_limits<double>::quiet_NaN());
	size_t timeCursor = 0;

	if (!mStored_Insulin.empty())
	{
		for (const auto& r : mStored_Insulin)
		{
			if (T == 0)
			{
				T = r.time_start;
				S1 += r.value;
			}
			else
			{
				while (T < r.time_start && T < *time.rbegin())
				{
					T += scgms::One_Minute;

					I += (S2 / (Vi * t_max)) - (ke * I);
					S2 += (S1 - S2) / t_max;
					S1 += 0 - S1 / t_max;

					if (timeCursor < time.size() && Is_Any_NaN(result[timeCursor]) && T >= time[timeCursor])
					{
						result[timeCursor] = I;
						timeCursor++;
					}
				}

				S1 += r.value;
			}
		}

		while (T < *time.rbegin())
		{
			T += scgms::One_Minute;

			I += (S2 / (Vi * t_max)) - (ke * I);
			S2 += (S1 - S2) / t_max;
			S1 += 0 - S1 / t_max;

			if (timeCursor < time.size() && Is_Any_NaN(result[timeCursor]) && T >= time[timeCursor])
			{
				result[timeCursor] = I;
				timeCursor++;
			}
		}

	}

	if (timeCursor < time.size() && Is_Any_NaN(result[timeCursor]) && T >= time[timeCursor])
	{
		result[timeCursor] = I;
		timeCursor++;
	}

	iob_acc = I;

	return iob_acc;
}

double CCGP_Prediction::Calc_COB_At(const std::vector<double>& time, std::vector<double>& result)
{
	double cob_acc = 0.0;

	//C(t) = (Dg * Ag * t * e^(-t/t_max)) / t_max^2

	constexpr double t_max = 40; //min
	constexpr double Ag = 0.8;

	constexpr double GlucMolar = 180.16; // glucose molar weight (g/mol)
	constexpr double gCHO_to_mmol = (1000 / GlucMolar); // 1000 -> g to mg, 180.16 - molar weight of glucose; g -> mmol

	std::fill(result.begin(), result.end(), 0);

	for (const auto& r : mStored_Carbs)
	{
		for (size_t i = 0; i < time.size(); i++)
		{
			if (time[i] >= r.time_start && time[i] < r.time_end)
			{
				const double t = (time[i] - r.time_start) / scgms::One_Minute; // device_time (rat time) to minutes

				const double Dg = gCHO_to_mmol * r.value;

				result[i] += (Dg * Ag * t * std::exp(-t / t_max)) / std::pow(t_max, 2.0);
				//cob_acc += Dg * Ag - (Dg * Ag * (t_max - std::exp(-t / t_max) * (t_max + t)) / t_max);
			}
		}
	}

	return cob_acc;
}

