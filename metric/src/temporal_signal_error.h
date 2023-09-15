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

#include "two_signals.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CTemporal_Signal_Error : public virtual CTwo_Signals, public virtual scgms::ISignal_Error_Inspection {
	protected:
		scgms::SMetric mMetric;
		scgms::SMetric mTemporal_Metric;

		double *mPromised_Metric = nullptr;
		uint64_t mPromised_Segment_id = scgms::All_Segments_Id;

		size_t mLevels_Required = 0;

		bool mEmit_Metric_As_Signal = false;
		bool mEmit_Last_Value_Only = false;

		bool mAllow_Multipoint_Affinity = false;

		double mLast_Emmitted_Time = std::numeric_limits<double>::quiet_NaN();
	
		virtual HRESULT On_Level_Added(const uint64_t segment_id, const double device_time) override final;
		HRESULT Emit_Metric_Signal(const uint64_t segment_id, const double device_time);
	
		double Calculate_Metric(const uint64_t segment_id);	//returns metric or NaN if could not calculate

		virtual void Do_Flush_Stats(std::wofstream stats_file) override final;
	protected:
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
	public:
		CTemporal_Signal_Error(scgms::IFilter *output);
		virtual ~CTemporal_Signal_Error();

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;	
		virtual HRESULT IfaceCalling Promise_Metric(const uint64_t segment_id, double* const metric_value, BOOL defer_to_dtor) override final;
		virtual HRESULT IfaceCalling Calculate_Signal_Error(const uint64_t segment_id, scgms::TSignal_Stats *absolute_error, scgms::TSignal_Stats *relative_error) override final;	
		//using CTwo_Signals::Get_Description;
		virtual HRESULT IfaceCalling Get_Description(wchar_t** const desc) override final { return CTwo_Signals::Get_Description(desc); };
};


#pragma warning( pop )
