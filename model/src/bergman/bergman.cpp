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

#include "bergman.h"
#include "../../../../common/rtl/SolverLib.h"

#include <type_traits>

CBergman_Discrete_Model::CBergman_Discrete_Model(glucose::IModel_Parameter_Vector *parameters, glucose::IFilter *output) : 
	mParameters(solver::Convert_Parameters<bergman_model::TParameters>(parameters, bergman_model::default_parameters.vector)), CBase_Filter(output) {
	//
}

CBergman_Discrete_Model::~CBergman_Discrete_Model() {

}

HRESULT CBergman_Discrete_Model::Do_Execute(glucose::UDevice_Event event) {
	return E_NOTIMPL;
}

HRESULT CBergman_Discrete_Model::Do_Configure(glucose::SFilter_Configuration configuration) {
	return E_NOTIMPL;	//configured in the constructor
}



HRESULT CBergman_Discrete_Model::Step(const double time_advance_delta) {
	return E_NOTIMPL;
}

HRESULT CBergman_Discrete_Model::Emit_Signal_Level(const GUID& signal_id, double device_time, double level) {
	glucose::UDevice_Event evt{ glucose::NDevice_Event_Code::Level };

	evt.device_id() = bergman_model::model_id;
	evt.device_time() = device_time;
	evt.level() = level;
	evt.signal_id() = signal_id;
	evt.segment_id() = reinterpret_cast<std::remove_reference<decltype(evt.segment_id())>::type>(this);

	return Send(evt);
}