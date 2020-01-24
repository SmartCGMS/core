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

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include <vector>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

 /*
  * Class that reads selected segments from the db produces the events
  * i.e., it mimicks CGMS
  */
class CSignal_Feedback : public scgms::CBase_Filter, public scgms::IFilter_Feedback_Sender {

	protected:
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

