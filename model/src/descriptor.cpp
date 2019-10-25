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

#include "descriptor.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/descriptor_utils.h"
#include "../../../common/rtl/manufactory.h"
#include "diffusion/Diffusion_v2_blood.h"
#include "diffusion/Diffusion_v2_ist.h"
#include "steil_rebrin/Steil_Rebrin_blood.h"
#include "bergman/bergman.h"

#include <vector>

namespace diffusion_v2_model {
	const GUID id = { 0x6645466a, 0x28d6, 0x4536,{ 0x9a, 0x38, 0xf, 0xd6, 0xea, 0x6f, 0xdb, 0x2d } }; // {6645466A-28D6-4536-9A38-0FD6EA6FDB2D}
	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble , glucose::NModel_Parameter_Value::mptDouble , glucose::NModel_Parameter_Value::mptTime, glucose::NModel_Parameter_Value::mptDouble , glucose::NModel_Parameter_Value::mptTime };
	const wchar_t *param_names[param_count] = {dsP, dsCg, dsC, dsDt, dsK, dsH};
	const wchar_t *param_columns[param_count] = { rsP_Column, rsCg_Column, rsC_Column, rsDt_Column, rsK_Column, rsH_Column };

	const double lower_bound[param_count] = {0.0, -0.5, -10.0, 0.0, -1.0, 0.0};
	const double upper_bound[param_count] = { 2.0, 0.0, 10.0, glucose::One_Hour, 0.0, glucose::One_Hour };

	const size_t signal_count = 2;
	
	const GUID signal_ids[signal_count] = { signal_Diffusion_v2_Blood, signal_Diffusion_v2_Ist };
	const wchar_t *signal_names[signal_count] = { dsBlood, dsInterstitial };
	const GUID reference_signal_ids[signal_count] = { glucose::signal_BG, glucose::signal_IG };

	glucose::TModel_Descriptor desc = {
		id,
		dsDiffusion_Model_v2,
		rsDiffusion_v2_Table,
		param_count,
		param_types,
		param_names,
		param_columns,
		lower_bound,
		default_parameters,
		upper_bound,
		signal_count,
		signal_ids,
		signal_names,
		reference_signal_ids
	};

}

namespace steil_rebrin {
	const GUID id = { 0x5fd93b14, 0xaaa9, 0x44d7,{ 0xa8, 0xb8, 0xc1, 0x58, 0x31, 0x83, 0x64, 0xbd } };  // {5FD93B14-AAA9-44D7-A8B8-C158318364BD}
	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble };
	const wchar_t *param_names[param_count] = { dsTau, dsAlpha, dsBeta, dsGamma };
	const wchar_t *param_columns[param_count] = { rsTau_Column, rsAlpha_Column, rsBeta_Column, rsGamma_Column };

	const double lower_bound[param_count] = { -1000.0, -1000.0, -1000.0, -1000.0 };
	const double upper_bound[param_count] = { 1000.0, 1000.0, 1000.0, 1000.0 };

	const size_t signal_count = 1;	
	const GUID signal_ids[signal_count] = { signal_Steil_Rebrin_Blood };
	const wchar_t *signal_names[signal_count] = { dsBlood };
	const GUID reference_signal_ids[signal_count] = { glucose::signal_BG };

	const glucose::TModel_Descriptor desc = {
		id,
		dsSteil_Rebrin,
		rsSteil_Rebrin_Table,
		param_count,
		param_types,
		param_names,
		param_columns,
		lower_bound,
		default_parameters,
		upper_bound,
		signal_count,
		signal_ids,
		signal_names,
		reference_signal_ids
	};

}


namespace steil_rebrin_diffusion_prediction {
	const GUID id = { 0x991bce49, 0x30de, 0x44e0, { 0xbb, 0x8, 0x44, 0x3e, 0x64, 0xb0, 0xc0, 0x7a } };

	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptTime,  glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble };
	const wchar_t *param_names[param_count] = { dsP, dsCg, dsC, dsDt, dsInv_G, dsTau };
	const wchar_t *param_columns[param_count] = { rsP_Column, rsCg_Column, rsC_Column, rsDt_Column, rsInv_G_Column, rsTau_Column};

	const double lower_bound[param_count] = { 0.0, -0.5, -10.0, 0.0, -1000.0, -1000.0 };
	const double upper_bound[param_count] = { 2.0, 0.0, 10.0, glucose::One_Hour, 1000.0, 1000.0};

	const size_t signal_count = 1;
	const GUID signal_ids[signal_count] = { signal_Steil_Rebrin_Diffusion_Prediction };
	const wchar_t *signal_names[signal_count] = { dsInterstitial };
	const GUID reference_signal_ids[signal_count] = { glucose::signal_IG };

	const glucose::TModel_Descriptor desc = {
		id,
		dsSteil_Rebrin_Diffusion_Prediction,
		rsSteil_Rebrin_Diffusion_Prediction_Table,
		param_count,
		param_types,
		param_names,
		param_columns,
		lower_bound,
		default_parameters,
		upper_bound,
		signal_count,
		signal_ids,
		signal_names,
		reference_signal_ids
	};

}

namespace diffusion_prediction {
	const GUID id = { 0xc5473eab, 0x32de, 0x49ce, { 0x90, 0xfc, 0xf2, 0xa2, 0xbd, 0x11, 0x3d, 0x88 } };

	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptTime, 
																	   glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptTime};

	const wchar_t *param_names[param_count] = { dsRetrospectiveP, dsRetrospectiveCg, dsRetrospectiveC, dsRetrospectiveDt, dsPredictiveP, dsPredictiveCg, dsPredictiveC, dsPredictiveDt };
	const wchar_t *param_columns[param_count] = { rsRetrospectiveP, rsRetrospectiveCg, rsRetrospectiveC, rsRetrospectiveDt, rsPredictiveP, rsPredictiveCg, rsPredictiveC, rsPredictiveDt };

	const double lower_bound[param_count] = { 0.0, -0.5, -10.0, 0.0, 0.0, -0.5, -10.0, 0.0 };
	const double upper_bound[param_count] = { 2.0, 0.0, 10.0, glucose::One_Hour, 2.0, 0.0, 10.0, glucose::One_Hour };

	const size_t signal_count = 1;

	const GUID signal_ids[signal_count] = { signal_Diffusion_Prediction };
	const wchar_t *signal_names[signal_count] = { dsInterstitial };
	const GUID reference_signal_ids[signal_count] = { glucose::signal_IG };

	const glucose::TModel_Descriptor desc = {
		id,
		dsDiffusion_Prediction,
		rsDiffusion_Prediction_Table,
		param_count,
		param_types,
		param_names,
		param_columns,
		lower_bound,
		default_parameters,
		upper_bound,
		signal_count,
		signal_ids,
		signal_names,
		reference_signal_ids
	};
}

namespace constant_model {
	const GUID id = { 0x637465fb, 0xfb6f, 0x4a05, { 0xbb, 0x13, 0xab, 0x2a, 0x59, 0xb9, 0x77, 0x4b } };	// {637465FB-FB6F-4A05-BB13-AB2A59B9774B}

	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptDouble };

	const wchar_t *param_names[param_count] = { dsConstantParam };
	const wchar_t *param_columns[param_count] = { rsConstantParam };

	const double lower_bound[param_count] = { 0.0 };
	const double upper_bound[param_count] = {  100.0 };

	const size_t signal_count = 1;

	const GUID signal_ids[signal_count] = { constant_model::signal_Constant };
	const wchar_t *signal_names[signal_count] = { dsConstant_Signal};
	const GUID reference_signal_ids[signal_count] = { glucose::signal_All };

	const glucose::TModel_Descriptor desc = {
		id,
		dsConstant_Model,
		rsConstant_Model,
		param_count,
		param_types,
		param_names,
		param_columns,
		lower_bound,
		default_parameters,
		upper_bound,
		signal_count,
		signal_ids,
		signal_names,
		reference_signal_ids
	};
}


namespace bergman_model {

	const wchar_t *model_param_ui_names[model_param_count] = {
		dsBergman_p1,
		dsBergman_p2,
		dsBergman_p3,
		dsBergman_p4,
		dsBergman_Vi,
		dsBergman_BW,
		dsBergman_VgDist,
		dsBergman_d1rate,
		dsBergman_d2rate,
		dsBergman_irate,
		dsBergman_Gb,
		dsBergman_Ib,
		dsBergman_G0,
		dsBergman_X0,
		dsBergman_I0,
		dsBergman_D10,
		dsBergman_D20,
		dsBergman_Isc0,
		dsBergman_Gsc0,
		dsBergman_BasalRate0,
		dsBergman_diff2_p,
		dsBergman_diff2_cg,
		dsBergman_diff2_c
	};

	const glucose::NModel_Parameter_Value model_param_types[model_param_count] = {
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble
	};

	constexpr size_t number_of_calculated_signals = 6;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		signal_Bergman_BG,
		signal_Bergman_IG,
		signal_Bergman_IOB,
		signal_Bergman_COB,
		signal_Bergman_Basal_Insulin,
		signal_Bergman_Insulin_Activity,
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		dsBergman_Signal_BG,
		dsBergman_Signal_IG,
		dsBergman_Signal_IOB,
		dsBergman_Signal_COB,
		dsBergman_Signal_Basal_Insulin,
		dsBergman_Signal_Insulin_Activity
	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		glucose::signal_BG,
		glucose::signal_IG,
		glucose::signal_IOB,
		glucose::signal_COB,
		glucose::signal_Delivered_Insulin_Basal,
		glucose::signal_Insulin_Activity,
	};

	glucose::TModel_Descriptor desc = {
		model_id,
		dsBergman_Minimal_Model,
		nullptr,
		model_param_count,
		model_param_types,
		model_param_ui_names,
		nullptr,
		lower_bounds.vector,
		default_parameters.vector,
		upper_bounds.vector,

		number_of_calculated_signals,
		calculated_signal_ids,
		calculated_signal_names,
		reference_signal_ids,
	};
}

const std::array<glucose::TModel_Descriptor, 6> model_descriptions = { { diffusion_v2_model::desc, 
																		 steil_rebrin::desc, steil_rebrin_diffusion_prediction::desc, diffusion_prediction::desc, 
																		 constant_model::desc,
																		 bergman_model::desc} };


HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end) {
	*begin = const_cast<glucose::TModel_Descriptor*>(model_descriptions.data());
	*end = *begin + model_descriptions.size();
	return S_OK;
}


HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, glucose::IModel_Parameter_Vector *parameters, glucose::IFilter *output, glucose::IDiscrete_Model **model) {
	if (*model_id == bergman_model::model_id) return Manufacture_Object<CBergman_Discrete_Model>(model, parameters, output);
		else return E_NOTIMPL;
}
