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

#include "configuration_link.h"

#include <scgms/rtl/manufactory.h>

CFilter_Configuration_Link::CFilter_Configuration_Link(const GUID &id) : mID(id) {
//	mParent_Path = std::make_unique<std::wstring>();
}

HRESULT IfaceCalling CFilter_Configuration_Link::Get_Filter_Id(GUID *id) {
	*id = mID;
	return S_OK;
}

HRESULT IfaceCalling CFilter_Configuration_Link::Set_Parent_Path(const wchar_t* path) {
	if ((!path) || (*path == 0)) {
		mParent_Path.clear();
		return S_OK;
	};
	
	mParent_Path = path;

	HRESULT rc = S_OK;
	for (auto param : mData) {
		if (!Succeeded(param->Set_Parent_Path(mParent_Path.c_str())))
			rc = E_UNEXPECTED;
	}

	return rc;
}

HRESULT IfaceCalling CFilter_Configuration_Link::Set_Variable(const wchar_t* name, const wchar_t* value) {
	if ((!name) || (*name == 0)) return E_INVALIDARG;
	if (name == CFilter_Parameter::mUnused_Variable_Name) return TYPE_E_AMBIGUOUSNAME;

	HRESULT rc = refcnt::internal::CVector_Container<scgms::IFilter_Parameter*>::empty();
	if (rc == S_FALSE) {
		for (auto param : mData) {
			if (!Succeeded(param->Set_Variable(name, value)))
				rc = E_UNEXPECTED;
		}
	}

	return rc;

}

HRESULT IfaceCalling CFilter_Configuration_Link::add(scgms::IFilter_Parameter** begin, scgms::IFilter_Parameter** end) {
	HRESULT rc = refcnt::internal::CVector_Container<scgms::IFilter_Parameter*>::add(begin, end);
	if (rc == S_OK)
		rc = Set_Parent_Path(mParent_Path.c_str());

	return rc;
}

DLL_EXPORT HRESULT IfaceCalling create_filter_configuration_link(const GUID* id, scgms::IFilter_Configuration_Link** link) {
	return Manufacture_Object<CFilter_Configuration_Link>(link, *id);
}
