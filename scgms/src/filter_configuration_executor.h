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

#include <scgms/iface/FilterIface.h>
#include <scgms/rtl/FilterLib.h>
#include <scgms/rtl/referencedImpl.h>

#include "executor.h"
#include "composite_filter.h"


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance 

class CFilter_Configuration_Executor : public virtual scgms::IFilter_Executor, public virtual refcnt::CReferenced {
	protected:
		std::recursive_mutex mCommunication_Guard;
		CComposite_Filter mComposite_Filter{ mCommunication_Guard };
		CTerminal_Filter mTerminal_Filter{ nullptr };

	public:
		CFilter_Configuration_Executor(scgms::IFilter *custom_output);
		virtual ~CFilter_Configuration_Executor();

		HRESULT Build_Filter_Chain(scgms::IFilter_Chain_Configuration *configuration, scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data, refcnt::Swstr_list& error_description);

		virtual HRESULT IfaceCalling Execute(scgms::IDevice_Event *event) override final;
		virtual HRESULT IfaceCalling Terminate(const BOOL wait_for_shutdown) override final;
};

#pragma warning( pop )

DLL_EXPORT HRESULT IfaceCalling execute_filter_configuration(scgms::IFilter_Chain_Configuration *configuration, scgms::TOn_Filter_Created on_filter_created, const void* on_filter_created_data, scgms::IFilter *custom_output, scgms::IFilter_Executor **executor, refcnt::wstr_list *error_description);
