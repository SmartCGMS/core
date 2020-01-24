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

#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DbLib.h"

#include <memory>
#include <thread>
#include <vector>

// helper struct for storing signal generation parameters
struct TGenerator_Signal_Parameters
{
	double offset;
	double amplitude;
	double period;

	double samplingPeriod;
};

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Class that generates sinus/cosinus functions as IG/BG signals
 */
class CSinCos_Generator : public scgms::CBase_Filter {
protected:
	// do we need to send shutdown after last value?
	bool mShutdownAfterLast = false;

	std::unique_ptr<std::thread> mGenerator_Thread;
	std::atomic<bool> mExit_Flag;

	// generator parameters for IG
	TGenerator_Signal_Parameters mIG_Params;
	// generator parameters for BG
	TGenerator_Signal_Parameters mBG_Params;
	// total time to be generated
	double mTotal_Time;

protected:
	void Run_Generator();

	bool Emit_Segment_Marker(uint64_t segment_id, bool start);
	bool Emit_Signal_Level(GUID signal_id, double time, double level, uint64_t segment_id);
	bool Emit_Shut_Down();

	void Start_Generator();
	void Terminate_Generator();
protected:
	virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
	HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
	CSinCos_Generator(scgms::IFilter *output);
	virtual ~CSinCos_Generator();
};

#pragma warning( pop )

