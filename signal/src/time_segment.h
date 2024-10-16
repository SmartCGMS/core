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

#pragma once

#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/DeviceLib.h>

#include <map>
#include <set>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CTime_Segment : public scgms::ITime_Segment, public virtual refcnt::CNotReferenced {
	protected:
		scgms::SFilter mOutput;
		const GUID mCalculated_Signal_Id;
		GUID mReference_Signal_Id = Invalid_GUID;
		const int64_t mSegment_id;

		scgms::SSignal mCalculated_Signal;
		std::map<GUID, scgms::SSignal> mSignals;
		scgms::SSignal Get_Signal_Internal(const GUID &signal_id);

		double mPrediction_Window;
		double mLast_Pending_time = std::numeric_limits<double>::quiet_NaN();	//it is faster to query it rather than to query the vector correctly
		std::set<double> mPending_Times;
		std::set<double> mEmitted_Times;	//to avoid duplicities in the output
		scgms::SModel_Parameter_Vector mWorking_Parameters;

	public:
		CTime_Segment(const int64_t segment_id, const GUID &calculated_signal_id, scgms::SModel_Parameter_Vector &working_parameters, const double prediction_window, scgms::SFilter output);
		virtual ~CTime_Segment() = default;

		virtual HRESULT IfaceCalling Get_Signal(const GUID *signal_id, scgms::ISignal **signal) override;

		bool Update_Level(const GUID &signal_id, const double level, const double time_stamp);
		bool Set_Parameters(scgms::SModel_Parameter_Vector parameters);
		scgms::SModel_Parameter_Vector Get_Parameters();
		bool Calculate(const std::vector<double> &times, std::vector<double> &levels);		//calculates using the working parameters
		void Emit_Levels_At_Pending_Times();
		void Clear_Data();
};

#pragma warning( pop )
