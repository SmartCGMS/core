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

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include "filter_parameter.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance 

class CFilter_Configuration_Link : public virtual refcnt::internal::CVector_Container<scgms::IFilter_Parameter*>, public virtual scgms::IFilter_Configuration_Link {
protected:
	const GUID mID;	
	std::wstring mParent_Path;	//for resolving relative paths; see CPersistent_Chain_Configuration for unique_ptr exaplanation
public:
	CFilter_Configuration_Link(const GUID &id);
	virtual ~CFilter_Configuration_Link() = default;

	virtual HRESULT IfaceCalling add(scgms::IFilter_Parameter* *begin, scgms::IFilter_Parameter* *end) override;

	virtual HRESULT IfaceCalling Get_Filter_Id(GUID *id) override final;
	virtual HRESULT IfaceCalling Set_Parent_Path(const wchar_t* path) override final;
	virtual HRESULT IfaceCalling Set_Variable(const wchar_t* name, const wchar_t* value) override final;
};


#pragma warning( pop )