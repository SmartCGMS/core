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
 * Univerzitni 8
 * 301 00, Pilsen
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

#include "sincos_generator.h"
#include "descriptor.h"

#include "../../../common/rtl/rattime.h"
#include "../../../common/lang/dstrings.h"

#include <cmath>

constexpr double PI = 3.141592653589793238462643;

CSinCos_Generator::CSinCos_Generator(scgms::IFilter *output) : CBase_Filter(output), mExit_Flag{ false } {
}

CSinCos_Generator::~CSinCos_Generator() {
	Terminate_Generator();
}

void CSinCos_Generator::Run_Generator() {
	const double startTime = Unix_Time_To_Rat_Time(time(nullptr));
	const double endTime = startTime + mTotal_Time;

	double nextIG = startTime + mIG_Params.samplingPeriod;
	double nextBG = startTime + mBG_Params.samplingPeriod;

	GUID signal;
	double level;
	double time = startTime;

	const uint64_t segment_id = 1; // probably no need to introduce more segments

	if (!Emit_Segment_Marker(segment_id, true))
		return;

	while (!mExit_Flag && time < endTime) {

		if (nextIG < nextBG)
		{
			signal = scgms::signal_IG;
			level = mIG_Params.amplitude * std::sin((nextIG - startTime)*(2*PI)/mIG_Params.period) + mIG_Params.offset;
			time = nextIG;
			nextIG += mIG_Params.samplingPeriod;
		}
		else
		{
			signal = scgms::signal_BG;
			level = mBG_Params.amplitude * std::cos((nextBG - startTime)*(2*PI)/mBG_Params.period) + mBG_Params.offset;
			time = nextBG;
			nextBG += mBG_Params.samplingPeriod;
		}

		if (!Emit_Signal_Level(signal, time, level, segment_id))
			break;

		if (signal == scgms::signal_BG)
			if (!Emit_Signal_Level(scgms::signal_Calibration, time, level, segment_id))
				break;
	}

	if (!Emit_Segment_Marker(segment_id, false))
		return;

	if (mShutdownAfterLast)
		Emit_Shut_Down();
}

bool CSinCos_Generator::Emit_Segment_Marker(uint64_t segment_id, bool start) {
	scgms::UDevice_Event evt{ start ? scgms::NDevice_Event_Code::Time_Segment_Start : scgms::NDevice_Event_Code::Time_Segment_Stop };
	evt.device_id() = sincos_generator::filter_id;
	evt.segment_id() = segment_id;
	return Send(evt) == S_OK;
}

bool CSinCos_Generator::Emit_Signal_Level(GUID signal_id, double time, double level, uint64_t segment_id) {
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Level };
	evt.device_time() = time;
	evt.signal_id() = signal_id;
	evt.segment_id() = segment_id;
	evt.level() = level;
	evt.device_id() = sincos_generator::filter_id;
	return Send(evt) == S_OK;
}

bool CSinCos_Generator::Emit_Shut_Down() {
	scgms::UDevice_Event evt{ scgms::NDevice_Event_Code::Shut_Down };
	evt.device_id() = sincos_generator::filter_id;
	return Send(evt) == S_OK;
}

HRESULT IfaceCalling CSinCos_Generator::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mIG_Params.offset = configuration.Read_Double(rsGen_IG_Offset);
	mIG_Params.amplitude = configuration.Read_Double(rsGen_IG_Amplitude);
	mIG_Params.period = configuration.Read_Double(rsGen_IG_Sin_Period);
	mIG_Params.samplingPeriod = configuration.Read_Double(rsGen_IG_Sampling_Period);

	mBG_Params.offset = configuration.Read_Double(rsGen_BG_Level_Offset);
	mBG_Params.amplitude = configuration.Read_Double(rsGen_BG_Amplitude);
	mBG_Params.period = configuration.Read_Double(rsGen_BG_Cos_Period);
	mBG_Params.samplingPeriod = configuration.Read_Double(rsGen_BG_Sampling_Period);

	mTotal_Time = configuration.Read_Double(rsGen_Total_Time);
	mShutdownAfterLast = configuration.Read_Bool(rsShutdown_After_Last);

	Start_Generator();

	return S_OK;
}

HRESULT IfaceCalling CSinCos_Generator::Do_Execute(scgms::UDevice_Event event) {
	if (event.event_code() == scgms::NDevice_Event_Code::Warm_Reset) {
		Terminate_Generator();
		Start_Generator();
	}

	return Send(event);
}

void CSinCos_Generator::Start_Generator()
{
	mExit_Flag = false;
	mGenerator_Thread = std::make_unique<std::thread>(&CSinCos_Generator::Run_Generator, this);
}

void CSinCos_Generator::Terminate_Generator()
{
	mExit_Flag = true;
	if (mGenerator_Thread->joinable())
		mGenerator_Thread->join();
}
