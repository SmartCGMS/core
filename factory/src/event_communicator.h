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

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance 


class CEvent_Communicator : public virtual glucose::IEvent_Receiver, public virtual glucose::IEvent_Sender, public virtual glucose::IFilter_Communicator, public virtual refcnt::CReferenced {
protected:
	glucose::IDevice_Event *mBuffered_Event = nullptr;	//buffer for exactly 1 event
	refcnt::SReferenced<glucose::IFilter_Communicator> mNext_Communicator;	
public:
	CEvent_Communicator(glucose::IFilter_Communicator* next_communicator);
	virtual ~CEvent_Communicator();

	virtual HRESULT IfaceCalling receive(glucose::IDevice_Event **event) override final;
	virtual HRESULT IfaceCalling send(glucose::IDevice_Event *event) override final;	
	virtual HRESULT IfaceCalling push_back(glucose::IDevice_Event *event) override;
};

#pragma warning( pop )