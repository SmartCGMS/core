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

#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/referencedImpl.h>

#include <vector>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

 /*
  * Class that reads selected segments from the db produces the events
  * i.e., it mimicks CGMS
  */
class CSignal_Feedback : public scgms::CBase_Filter, public scgms::IFilter_Feedback_Sender {
	protected:
		std::atomic<size_t> mStack_Counter{ 0 };	//counter used to detect possibly infinity loop
		const size_t mMaximum_Stack_Depth = 10;		//maximum depth of Do_Execute

		scgms::SFilter_Feedback_Receiver mReceiver;
		std::wstring mFeedback_Name;
		GUID mSignal_ID = Invalid_GUID;
		bool mForward_Clone = false;

	protected:
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CSignal_Feedback(scgms::IFilter *output);
		virtual ~CSignal_Feedback();

		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override;
		virtual HRESULT IfaceCalling Sink(scgms::IFilter_Feedback_Receiver *receiver) override final;
		virtual HRESULT IfaceCalling Name(wchar_t** const name) override final;
};

#pragma warning( pop )
