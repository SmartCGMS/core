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
#include "uva_padova/uva_padova.h"
#include "bolus/insulin_bolus.h"

#include <vector>

namespace diffusion_v2_model {
	const GUID id = { 0x6645466a, 0x28d6, 0x4536,{ 0x9a, 0x38, 0xf, 0xd6, 0xea, 0x6f, 0xdb, 0x2d } }; // {6645466A-28D6-4536-9A38-0FD6EA6FDB2D}
	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble , scgms::NModel_Parameter_Value::mptDouble , scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime };
	const wchar_t *param_names[param_count] = {dsP, dsCg, dsC, dsDt, dsK, dsH};
	const wchar_t *param_columns[param_count] = { rsP_Column, rsCg_Column, rsC_Column, rsDt_Column, rsK_Column, rsH_Column };

	const double lower_bound[param_count] = {0.0, -0.5, -10.0, 0.0, -1.0, 0.0};
	const double upper_bound[param_count] = { 2.0, 0.0, 10.0, scgms::One_Hour, 0.0, scgms::One_Hour };

	const size_t signal_count = 2;
	
	const GUID signal_ids[signal_count] = { signal_Diffusion_v2_Blood, signal_Diffusion_v2_Ist };	
	const GUID reference_signal_ids[signal_count] = { scgms::signal_BG, scgms::signal_IG };

	const scgms::TModel_Descriptor desc = {
		id,
		scgms::NModel_Flags::Signal_Model,
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
		reference_signal_ids
	};

	
	const std::wstring blood_desc = std::wstring{ dsDiffusion_Model_v2 } +L" - " + dsBlood;
	const scgms::TSignal_Descriptor bg_desc { signal_Diffusion_v2_Blood, blood_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0000, 0xFFFF0000, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

	const std::wstring ist_desc = std::wstring{ dsDiffusion_Model_v2 } +L" - " + dsInterstitial;
	const scgms::TSignal_Descriptor ig_desc{ signal_Diffusion_v2_Ist, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

}

namespace steil_rebrin {
	const GUID id = { 0x5fd93b14, 0xaaa9, 0x44d7,{ 0xa8, 0xb8, 0xc1, 0x58, 0x31, 0x83, 0x64, 0xbd } };  // {5FD93B14-AAA9-44D7-A8B8-C158318364BD}
	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble };
	const wchar_t *param_names[param_count] = { dsTau, dsAlpha, dsBeta, dsGamma };
	const wchar_t *param_columns[param_count] = { rsTau_Column, rsAlpha_Column, rsBeta_Column, rsGamma_Column };

	const double lower_bound[param_count] = { -1000.0, -1000.0, -1000.0, -1000.0 };
	const double upper_bound[param_count] = { 1000.0, 1000.0, 1000.0, 1000.0 };

	const size_t signal_count = 1;	
	const GUID signal_ids[signal_count] = { signal_Steil_Rebrin_Blood };
	const GUID reference_signal_ids[signal_count] = { scgms::signal_BG };

	const scgms::TModel_Descriptor desc = {
		id,
		scgms::NModel_Flags::Signal_Model,
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
		reference_signal_ids
	};

	const std::wstring blood_desc = std::wstring{ dsSteil_Rebrin } +L" - " + dsBlood;
	const scgms::TSignal_Descriptor bg_desc{ signal_Steil_Rebrin_Blood, blood_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0000, 0xFFFF0000, scgms::NSignal_Visualization::smooth };

}


namespace steil_rebrin_diffusion_prediction {
	const GUID id = { 0x991bce49, 0x30de, 0x44e0, { 0xbb, 0x8, 0x44, 0x3e, 0x64, 0xb0, 0xc0, 0x7a } };

	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime,  scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble };
	const wchar_t *param_names[param_count] = { dsP, dsCg, dsC, dsDt, dsInv_G, dsTau };
	const wchar_t *param_columns[param_count] = { rsP_Column, rsCg_Column, rsC_Column, rsDt_Column, rsInv_G_Column, rsTau_Column};

	const double lower_bound[param_count] = { 0.0, -0.5, -10.0, 0.0, -1000.0, -1000.0 };
	const double upper_bound[param_count] = { 2.0, 0.0, 10.0, scgms::One_Hour, 1000.0, 1000.0};

	const size_t signal_count = 1;
	const GUID signal_ids[signal_count] = { signal_Steil_Rebrin_Diffusion_Prediction };
	const GUID reference_signal_ids[signal_count] = { scgms::signal_IG };

	const scgms::TModel_Descriptor desc = {
		id,
		scgms::NModel_Flags::Signal_Model,
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
		reference_signal_ids
	};

	const std::wstring ist_desc = std::wstring{ dsSteil_Rebrin_Diffusion_Prediction } +L" - " + dsInterstitial;
	const scgms::TSignal_Descriptor ig_desc{ signal_Steil_Rebrin_Diffusion_Prediction, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
}

namespace diffusion_prediction {
	const GUID id = { 0xc5473eab, 0x32de, 0x49ce, { 0x90, 0xfc, 0xf2, 0xa2, 0xbd, 0x11, 0x3d, 0x88 } };

	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, 
																	   scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime};

	const wchar_t *param_names[param_count] = { dsRetrospectiveP, dsRetrospectiveCg, dsRetrospectiveC, dsRetrospectiveDt, dsPredictiveP, dsPredictiveCg, dsPredictiveC, dsPredictiveDt };
	const wchar_t *param_columns[param_count] = { rsRetrospectiveP, rsRetrospectiveCg, rsRetrospectiveC, rsRetrospectiveDt, rsPredictiveP, rsPredictiveCg, rsPredictiveC, rsPredictiveDt };

	const double lower_bound[param_count] = { 0.0, -0.5, -10.0, 0.0, 0.0, -0.5, -10.0, 0.0 };
	const double upper_bound[param_count] = { 2.0, 0.0, 10.0, scgms::One_Hour, 2.0, 0.0, 10.0, scgms::One_Hour };

	const size_t signal_count = 1;

	const GUID signal_ids[signal_count] = { signal_Diffusion_Prediction };
	const GUID reference_signal_ids[signal_count] = { scgms::signal_IG };

	const scgms::TModel_Descriptor desc = {
		id,
		scgms::NModel_Flags::Signal_Model,
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
		reference_signal_ids
	};

	const std::wstring ist_desc = std::wstring{ dsDiffusion_Prediction } +L" - " + dsInterstitial;
	const scgms::TSignal_Descriptor ig_desc{ signal_Diffusion_Prediction, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
}

namespace constant_model {
	const GUID id = { 0x637465fb, 0xfb6f, 0x4a05, { 0xbb, 0x13, 0xab, 0x2a, 0x59, 0xb9, 0x77, 0x4b } };	// {637465FB-FB6F-4A05-BB13-AB2A59B9774B}

	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble };

	const wchar_t *param_names[param_count] = { dsConstantParam };
	const wchar_t *param_columns[param_count] = { rsConstantParam };

	const double lower_bound[param_count] = { 0.0 };
	const double upper_bound[param_count] = {  100.0 };

	const size_t signal_count = 1;

	const GUID signal_ids[signal_count] = { constant_model::signal_Constant };
	const wchar_t *signal_names[signal_count] = { dsConstant_Signal};
	const GUID reference_signal_ids[signal_count] = { scgms::signal_All };

	const scgms::TModel_Descriptor desc = {
		id,
		scgms::NModel_Flags::Signal_Model,
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
		reference_signal_ids
	};


	const scgms::TSignal_Descriptor const_desc{ constant_model::signal_Constant, dsConstant_Signal, L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
}


namespace bergman_model {

	const wchar_t *model_param_ui_names[model_param_count] = {
		dsBergman_p1,
		dsBergman_p2,
		dsBergman_p3,
		dsBergman_p4,
		dsBergman_k12,
		dsBergman_k21,
		dsBergman_Vi,
		dsBergman_BW,
		dsBergman_VgDist,
		dsBergman_d1rate,
		dsBergman_d2rate,
		dsBergman_irate,
		dsBergman_Qb,
		dsBergman_Ib,
		dsBergman_Q10,
		dsBergman_Q20,
		dsBergman_X0,
		dsBergman_I0,
		dsBergman_D10,
		dsBergman_D20,
		dsBergman_Isc0,
		dsBergman_Gsc0,
		dsBergman_BasalRate0,
		dsBergman_diff2_p,
		dsBergman_diff2_cg,
		dsBergman_diff2_c,
		dsBergman_diff2_dt,
		dsBergman_diff2_k,
		dsBergman_diff2_h,
		dsBergman_Ag
	};

	const scgms::NModel_Parameter_Value model_param_types[model_param_count] = {
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptTime,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptTime,
		scgms::NModel_Parameter_Value::mptDouble
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
		scgms::signal_BG,
		scgms::signal_IG,
		scgms::signal_IOB,
		scgms::signal_COB,
		scgms::signal_Delivered_Insulin_Basal_Rate,
		scgms::signal_Insulin_Activity,
	};

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
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
		reference_signal_ids,
	};

	const std::wstring blood_desc = std::wstring{ dsBergman_Minimal_Model } +L" - " + dsBlood;
	const scgms::TSignal_Descriptor bg_desc{ signal_Bergman_BG, blood_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0000, 0xFFFF0000, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

	const std::wstring ist_desc = std::wstring{ dsBergman_Minimal_Model } +L" - " + dsInterstitial;
	const scgms::TSignal_Descriptor ig_desc{ signal_Bergman_IG, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

	const std::wstring iob_str_desc = std::wstring{ dsBergman_Minimal_Model } +L" - " + dsSignal_GUI_Name_IOB;
	const scgms::TSignal_Descriptor iob_desc{ signal_Bergman_IOB, iob_str_desc.c_str(), L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

	const std::wstring cob_str_desc = std::wstring{ dsBergman_Minimal_Model } +L" - " + dsSignal_GUI_Name_COB;
	const scgms::TSignal_Descriptor cob_desc{ signal_Bergman_COB, cob_str_desc.c_str(), L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF,scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

	const std::wstring basal_insulin_str_desc = std::wstring{ dsBergman_Minimal_Model } +L" - " + dsSignal_GUI_Name_Basal_Insulin;
	const scgms::TSignal_Descriptor basal_insulin_desc{ signal_Bergman_Basal_Insulin, basal_insulin_str_desc.c_str(), L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

	const std::wstring insulin_activity_str_desc = std::wstring{ dsBergman_Minimal_Model } +L" - " + dsSignal_GUI_Name_Insulin_Activity;
	const scgms::TSignal_Descriptor insulin_activity_desc{ signal_Bergman_Insulin_Activity, insulin_activity_str_desc.c_str(), L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
}

namespace uva_padova_model {

	const wchar_t *model_param_ui_names[model_param_count] = {
		// TODO: move to dstrings
		L"Qsto1_0",L"Qsto2_0",L"Qgut_0",L"Gp_0",L"Gt_0",L"Ip_0",L"X_0",L"I_0",L"XL_0",L"Il_0",L"Isc1_0",L"Isc2_0",L"Gs_0",
		L"BW",L"Gb",L"Ib",
		L"kabs",L"kmax",L"kmin",L"b",L"d",L"Vg",L"Vi",L"Vmx",L"Km0",L"k2",L"k1",L"p2u",L"m1",L"m2",L"m4",
		L"m30",L"ki",L"kp2",L"kp3",L"f",L"ke1",L"ke2",L"Fsnc",L"Vm0",L"kd",L"ksc",L"ka1",L"ka2", L"u2ss",L"kp1",
		L"kh1",L"kh2",L"kh3",L"SRHb",L"n",L"rho",L"sigma1",L"sigma2",L"delta",L"xi",
		L"kH",L"Hb",L"XH_0",L"Hsc1b",L"Hsc2b"
	};

	const scgms::NModel_Parameter_Value model_param_types[model_param_count] = {
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,

		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble
	};

	constexpr size_t number_of_calculated_signals = 2;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		signal_UVa_Padova_IG,
		signal_UVa_Padova_BG,
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		dsUVa_Padova_IG,
		dsUVa_Padova_BG,
	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		scgms::signal_IG,
		scgms::signal_BG
	};

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		dsUVa_Padova_Model,
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
		reference_signal_ids,
	};

	const std::wstring ist_desc = std::wstring{ dsUVa_Padova_Model } + L" - " + dsInterstitial;
	const scgms::TSignal_Descriptor ig_desc{ signal_UVa_Padova_IG, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

	const std::wstring bgs_desc = std::wstring{ dsUVa_Padova_Model } + L" - " + dsBlood;
	const scgms::TSignal_Descriptor bg_desc{ signal_UVa_Padova_BG, bgs_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0088, 0xFFFF0088, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
}

namespace insulin_bolus {
	const GUID model_id = { 0x17f68d4, 0x5161, 0x454c, { 0x93, 0xd9, 0x96, 0x9d, 0xe5, 0x78, 0x4d, 0xd9 } };// {017F68D4-5161-454C-93D9-969DE5784DD9}

	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble };

	const wchar_t *param_names[param_count] = { dsCSR };
	const wchar_t *param_columns[param_count] = { rsCSR };

	const double lower_bound[param_count] = { 0.0 };
	const double upper_bound[param_count] = { 2.0 };

	const size_t signal_count = 1;

	const GUID signal_ids[signal_count] = { scgms::signal_Requested_Insulin_Bolus };
	const wchar_t *signal_names[signal_count] = { dsCalculated_Bolus_Insulin };
	const GUID reference_signal_ids[signal_count] = { scgms::signal_Delivered_Insulin_Bolus };

	const scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		dsBolus_Calculator,
		rsBolus_Calculator,
		param_count,
		param_types,
		param_names,
		param_columns,
		lower_bound,
		default_parameters,
		upper_bound,
		signal_count,
		signal_ids,		
		reference_signal_ids
	};
	
	//described elsewere, in the signal.dll
}

namespace const_isf {
	const GUID id = { 0x2399912b, 0x54a3, 0x4a54, { 0xad, 0x70, 0xf2, 0xa5, 0xb6, 0xe8, 0x1e, 0x2 } };	// {2399912B-54A3-4A54-AD70-F2A5B6E81E02}

	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble };

	const wchar_t *param_names[param_count] = { dsISF };
	const wchar_t *param_columns[param_count] = { rsISF };

	const double lower_bound[param_count] = { 0.0 };
	const double upper_bound[param_count] = { 5.0 };

	const size_t signal_count = 1;

	const GUID signal_ids[signal_count] = { const_isf_signal_id };
	const wchar_t *signal_names[signal_count] = { dsConst_ISF };
	const GUID reference_signal_ids[signal_count] = { scgms::signal_IOB }; // maybe IOB is a good fit

	const scgms::TModel_Descriptor desc = {
		id,
		scgms::NModel_Flags::Signal_Model,
		dsConst_ISF_Model,
		rsConst_ISF,
		param_count,
		param_types,
		param_names,
		param_columns,
		lower_bound,
		default_parameters,
		upper_bound,
		signal_count,
		signal_ids,
		reference_signal_ids
	};

	//signal not described because not needed to show?
}

namespace const_cr {
	const GUID id = { 0xefd71f27, 0x6f47, 0x463d, { 0xb1, 0x69, 0x96, 0x82, 0xff, 0x67, 0x63, 0xf } };// {EFD71F27-6F47-463D-B169-9682FF67630F}

	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble };

	const wchar_t *param_names[param_count] = { dsCSR };
	const wchar_t *param_columns[param_count] = { rsCSR };

	const double lower_bound[param_count] = { 0.0 };
	const double upper_bound[param_count] = { 2.0 };

	const size_t signal_count = 1;

	const GUID signal_ids[signal_count] = { const_cr_signal_id };
	const wchar_t *signal_names[signal_count] = { dsConst_CR };
	const GUID reference_signal_ids[signal_count] = { scgms::signal_IOB }; // maybe IOB is a good fit

	const scgms::TModel_Descriptor desc = {
		id,
		scgms::NModel_Flags::Signal_Model,
		dsConst_CR_Model,
		rsConst_CR,
		param_count,
		param_types,
		param_names,
		param_columns,
		lower_bound,
		default_parameters,
		upper_bound,
		signal_count,
		signal_ids,
		reference_signal_ids
	};

	//signal not described because not needed to show?
}


const std::array<scgms::TModel_Descriptor, 10> model_descriptions = { { diffusion_v2_model::desc, 
																		 steil_rebrin::desc, steil_rebrin_diffusion_prediction::desc, diffusion_prediction::desc, 
																		 constant_model::desc,
																		 bergman_model::desc,
																		 uva_padova_model::desc,
																		 insulin_bolus::desc,
																		 const_isf::desc, const_cr::desc } };


const std::array<scgms::TSignal_Descriptor, 14> signals_descriptors = { {diffusion_v2_model::bg_desc, diffusion_v2_model::ig_desc, steil_rebrin::bg_desc, 
																		 steil_rebrin_diffusion_prediction::ig_desc, diffusion_prediction::ig_desc, 
																		 constant_model::const_desc,
																		 bergman_model::bg_desc, bergman_model::ig_desc, bergman_model::iob_desc, bergman_model::cob_desc, bergman_model::basal_insulin_desc, bergman_model::insulin_activity_desc,
																		 uva_padova_model::ig_desc, uva_padova_model::bg_desc
																		}};

HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end) {
	*begin = const_cast<scgms::TModel_Descriptor*>(model_descriptions.data());
	*end = *begin + model_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor * *begin, scgms::TSignal_Descriptor * *end) {
	*begin = const_cast<scgms::TSignal_Descriptor*>(signals_descriptors.data());
	*end = *begin + signals_descriptors.size();
	return S_OK;
}



HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model) {
	if (*model_id == bergman_model::model_id) return Manufacture_Object<CBergman_Discrete_Model>(model, parameters, output);
	else if (*model_id == uva_padova_model::model_id) return Manufacture_Object<CUVa_Padova_Discrete_Model>(model, parameters, output);
	else if (*model_id == insulin_bolus::model_id) return Manufacture_Object<CDiscrete_Insulin_Bolus_Calculator>(model, parameters, output);
		else return E_NOTIMPL;
}
