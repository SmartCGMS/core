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
#include <scgms/rtl/referencedImpl.h>

#include <memory>
#include <thread>
#include <map>
#include <atomic>
#include <vector>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

namespace signal_generator_internal {
	class CSynchronized_Generator : public scgms::IFilter, public refcnt::CNotReferenced {
		protected:
			uint64_t mSegment_Id = scgms::Invalid_Segment_Id;
			bool mCatching_Up = false;
			GUID mSync_Signal = Invalid_GUID;
			double mTime_To_Catch_Up = 0.0;
			double mFixed_Stepping = 5.0*scgms::One_Minute;
			double mLast_Device_Time = std::numeric_limits<double>::quiet_NaN();
			scgms::SDiscrete_Model mSync_Model;

			scgms::IFilter *mDirect_Output;	//output we use if the event was for a single event
			scgms::IFilter *mChained_Output; //output we use if the event was shutdown all for all segments

		public:
			CSynchronized_Generator(scgms::IFilter *direct_output, scgms::IFilter *chained_output, const uint64_t segment_id);
			virtual ~CSynchronized_Generator();

			HRESULT Execute_Sync(scgms::UDevice_Event &event);

			virtual HRESULT IfaceCalling Configure(scgms::IFilter_Configuration* configuration, refcnt::wstr_list* error_description) override final;
			virtual HRESULT IfaceCalling Execute(scgms::IDevice_Event *event) override final;
	};
}

/*
 * Filter class for generating signals using a specific model 
 */
class CSignal_Generator : public scgms::CBase_Filter, public scgms::IFilter_Feedback_Receiver {
	protected:
		bool mSync_To_Signal = false;	
		double mFixed_Stepping = 5.0*scgms::One_Minute;
		double mMax_Time = 24.0 * scgms::One_Hour;			//maximum time, for which the generator can run
		double mTotal_Time = 0.0;	//time for which the generator runs	
		bool mEmit_Shutdown = true;
		scgms::SDiscrete_Model mAsync_Model;
		scgms::SFilter_Configuration mSync_Configuration;

		signal_generator_internal::CSynchronized_Generator* mLast_Sync_Generator = nullptr;
		using TSync_Model = std::unique_ptr<signal_generator_internal::CSynchronized_Generator>;
		std::map<uint64_t, TSync_Model> mSync_Models;

		std::wstring mFeedback_Name;
		std::unique_ptr<std::thread> mThread;
		std::atomic<bool> mQuitting = false;
		void Stop_Generator(bool wait);

		size_t mNumber_Of_Segment_Specific_Parameters = 0;
		size_t mCurrent_Segment_Idx = 0;
		scgms::SFilter_Configuration mOriginal_Configuration;
		std::vector<double> mSegment_Agnostic_Parameters, mSegment_Agnostic_Lower_Bound, mSegment_Agnostic_Upper_Bound;
		std::vector<std::vector<double>> mSegment_Specific_Parameters, mSegment_Specific_Lower_Bound, mSegment_Specific_Upper_Bound;

	protected:
		void Update_Sync_Configuration_Parameters();	//increments the current segment and expands mSegment_Specific_Parameters if needed

		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CSignal_Generator(scgms::IFilter *output);
		virtual ~CSignal_Generator();

		virtual HRESULT IfaceCalling Name(wchar_t** const name) override final;
		virtual HRESULT IfaceCalling QueryInterface(const GUID* riid, void** ppvObj) override;
};

#pragma warning( pop )
