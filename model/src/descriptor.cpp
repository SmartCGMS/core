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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "descriptor.h"
#include "../../../common/utils/descriptor_utils.h"
#include "pattern_prediction/pattern_prediction_descriptor.h"

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"
#include "diffusion/Diffusion_v2_blood.h"
#include "diffusion/Diffusion_v2_ist.h"
#include "steil_rebrin/Steil_Rebrin_blood.h"
#include "bergman/bergman.h"
#include "uva_padova/uva_padova_s2013.h"
#include "uva_padova/uva_padova_s2017.h"
#include "bolus/insulin_bolus.h"
#include "pattern_prediction/pattern_prediction.h"
#include "samadi/samadi.h"
#include "gct/gct.h"
#include "gct2/gct2.h"
#include "p559/p559.h"
#include "aim_ge/aim_ge.h"
#include "enhacement/data_enhacement.h"
#include "cgp_pred/cgp_pred.h"
#include "bases/bases.h"

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
		0,
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
	const scgms::TSignal_Descriptor bg_desc { signal_Diffusion_v2_Blood, blood_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0000, 0xFFFF0000, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring ist_desc = std::wstring{ dsDiffusion_Model_v2 } +L" - " + dsInterstitial;
	const scgms::TSignal_Descriptor ig_desc{ signal_Diffusion_v2_Ist, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

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
		0,
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
	const scgms::TSignal_Descriptor bg_desc{ signal_Steil_Rebrin_Blood, blood_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0000, 0xFFFF0000, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

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
		0,
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
	const scgms::TSignal_Descriptor ig_desc{ signal_Steil_Rebrin_Diffusion_Prediction, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
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
		0,
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
	const scgms::TSignal_Descriptor ig_desc{ signal_Diffusion_Prediction, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
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
		0,
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


	const scgms::TSignal_Descriptor const_desc{ constant_model::signal_Constant, dsConstant_Signal, L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
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
		0,
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
	const scgms::TSignal_Descriptor bg_desc{ signal_Bergman_BG, blood_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0000, 0xFFFF0000, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring ist_desc = std::wstring{ dsBergman_Minimal_Model } +L" - " + dsInterstitial;
	const scgms::TSignal_Descriptor ig_desc{ signal_Bergman_IG, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring iob_str_desc = std::wstring{ dsBergman_Minimal_Model } +L" - " + dsSignal_GUI_Name_IOB;
	const scgms::TSignal_Descriptor iob_desc{ signal_Bergman_IOB, iob_str_desc.c_str(), L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring cob_str_desc = std::wstring{ dsBergman_Minimal_Model } +L" - " + dsSignal_GUI_Name_COB;
	const scgms::TSignal_Descriptor cob_desc{ signal_Bergman_COB, cob_str_desc.c_str(), L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF,scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 0.1 };

	const std::wstring basal_insulin_str_desc = std::wstring{ dsBergman_Minimal_Model } +L" - " + dsSignal_GUI_Name_Basal_Insulin;
	const scgms::TSignal_Descriptor basal_insulin_desc{ signal_Bergman_Basal_Insulin, basal_insulin_str_desc.c_str(), L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring insulin_activity_str_desc = std::wstring{ dsBergman_Minimal_Model } +L" - " + dsSignal_GUI_Name_Insulin_Activity;
	const scgms::TSignal_Descriptor insulin_activity_desc{ signal_Bergman_Insulin_Activity, insulin_activity_str_desc.c_str(), L"", scgms::NSignal_Unit::Other, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
}

namespace uva_padova_S2013 { //DOI: 10.1177/1932296813514502

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

	constexpr size_t number_of_calculated_signals = 3;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		uva_padova_S2013::signal_IG,
		uva_padova_S2013::signal_BG,
		uva_padova_S2013::signal_Delivered_Insulin,
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		dsUVa_Padova_IG,
		dsUVa_Padova_BG,
		dsUVa_Padova_Delivered_Insulin,

	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		scgms::signal_IG,
		scgms::signal_BG,
		scgms::signal_Delivered_Insulin_Total
	};

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		dsUVa_Padova_S2013,
		nullptr,
		model_param_count,
		13,
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

	const std::wstring ist_desc = std::wstring{ dsUVa_Padova_IG };
	const scgms::TSignal_Descriptor ig_desc{ uva_padova_S2013::signal_IG, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring bgs_desc = std::wstring{ dsUVa_Padova_BG };
	const scgms::TSignal_Descriptor bg_desc{ uva_padova_S2013::signal_BG, bgs_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0088, 0xFFFF0088, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring inss_desc = std::wstring{ dsUVa_Padova_Delivered_Insulin };
	const scgms::TSignal_Descriptor ins_desc{ uva_padova_S2013::signal_Delivered_Insulin, inss_desc.c_str(), dsU, scgms::NSignal_Unit::U_insulin, 0xFF450098, 0xFF450098, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
}

namespace uva_padova_S2017 { //DOI: 10.1177/1932296818757747

	const wchar_t *model_param_ui_names[model_param_count] = {
		// TODO: move to dstrings
		L"Qsto1_0",L"Qsto2_0",L"Qgut_0",L"Gp_0",L"Gt_0",L"Ip_0",L"X_0",L"I_0",L"XL_0",L"Il_0",L"Isc1_0",L"Isc2_0",L"Iid1_0",L"Iid2_0",L"Iih_0",L"Gsc_0",
		L"BW",L"Gb",L"Ib",
		L"kabs",L"kmax",L"kmin",L"beta",L"Vg",L"Vi",L"Vmx",L"Km0",L"k2",L"k1",L"p2u",L"m1",L"m2",L"m4",
		L"m30",L"ki",L"kp2",L"kp3",L"f",L"ke1",L"ke2",L"Fsnc",L"Vm0",L"kd",L"ka1",L"ka2", L"u2ss",L"kp1",
		L"kh1",L"kh2",L"kh3",L"SRHb",L"n",L"rho",L"sigma",L"delta",L"xi",
		L"kH",L"Hb",L"XH_0",L"Hsc1b",L"Hsc2b",L"kir",L"ka",L"kaIih",L"r1",L"r2",L"m3",L"alpha",L"c",L"FIih",L"Ts",
		L"b1",L"b2",L"a2",
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

		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
	};

	constexpr size_t number_of_calculated_signals = 5;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		uva_padova_S2017::signal_IG,
		uva_padova_S2017::signal_BG,
		uva_padova_S2017::signal_Delivered_Insulin,
		uva_padova_S2017::signal_IOB,
		uva_padova_S2017::signal_COB,
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		dsUVa_Padova_S2017_IG,
		dsUVa_Padova_S2017_BG,
		dsUVa_Padova_S2017_Delivered_Insulin,
		dsUVa_Padova_S2017_IOB,
		dsUVa_Padova_S2017_COB,
	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		scgms::signal_IG,
		scgms::signal_BG,
		scgms::signal_Delivered_Insulin_Total,
		scgms::signal_IOB,
		scgms::signal_COB,
	};

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		dsUVa_Padova_S2017,
		nullptr,
		model_param_count,
		16,
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

	const std::wstring ist_desc = std::wstring{ dsUVa_Padova_S2017_IG };
	const scgms::TSignal_Descriptor ig_desc{ uva_padova_S2017::signal_IG, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring bgs_desc = std::wstring{ dsUVa_Padova_S2017_BG };
	const scgms::TSignal_Descriptor bg_desc{ uva_padova_S2017::signal_BG, bgs_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0088, 0xFFFF0088, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring inss_desc = std::wstring{ dsUVa_Padova_S2017_Delivered_Insulin };
	const scgms::TSignal_Descriptor ins_desc{ uva_padova_S2017::signal_Delivered_Insulin, inss_desc.c_str(), dsU, scgms::NSignal_Unit::U_insulin, 0xFF450098, 0xFF450098, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring iobs_desc = std::wstring{ dsUVa_Padova_S2017_IOB };
	const scgms::TSignal_Descriptor iob_desc{ uva_padova_S2017::signal_IOB, iobs_desc.c_str(), dsU, scgms::NSignal_Unit::U_insulin, 0xFF456898, 0xFF456898, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring cobs_desc = std::wstring{ dsUVa_Padova_S2017_COB };
	const scgms::TSignal_Descriptor cob_desc{ uva_padova_S2017::signal_COB, cobs_desc.c_str(), dsU, scgms::NSignal_Unit::U_insulin, 0xFF45CC98, 0xFF45CC98, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 0.1 };
}

namespace gct_model {

	const wchar_t* model_param_ui_names[model_param_count] = {
		// TODO: move to dstrings
		L"Q1_0", L"Q2_0", L"Qsc_0", L"I_0", L"Isc_0", L"X_0", L"D1_0", L"D2_0",
		L"Vq", L"Vqsc", L"Vi", L"Q1b", L"Gthr", L"GIthr",
		L"q12", L"q1sc", L"ix", L"xi", L"d12", L"d2q1",L"isc2i",
		L"q1e", L"q1ee", L"q1e_thr", L"xe",
		L"q1p", L"q1pe", L"ip",
		L"e_pa", L"e_ua", L"e_pe", L"e_ue", L"q_ep", L"q_eu",
		L"Ag", L"t_d", L"t_i"
	};

	const scgms::NModel_Parameter_Value model_param_types[model_param_count] = {
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptTime
	};

	constexpr size_t number_of_calculated_signals = 5;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		gct_model::signal_IG,
		gct_model::signal_BG,
		gct_model::signal_Delivered_Insulin,
		gct_model::signal_IOB,
		gct_model::signal_COB,
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		dsGCT_Model_v1_IG,
		dsGCT_Model_v1_BG,
		dsGCT_Model_v1_Delivered_Insulin,
		dsGCT_Model_v1_IOB,
		dsGCT_Model_v1_COB,
	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		scgms::signal_IG,
		scgms::signal_BG,
		scgms::signal_Delivered_Insulin_Total,
		scgms::signal_IOB,
		scgms::signal_COB,
	};

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		dsGCT_Model_v1,
		nullptr,
		model_param_count,
		8,
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

	const scgms::TSignal_Descriptor ig_desc{ gct_model::signal_IG, dsGCT_Model_v1_IG, dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
	const scgms::TSignal_Descriptor bg_desc{ gct_model::signal_BG, dsGCT_Model_v1_BG, dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0088, 0xFFFF0088, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
	const scgms::TSignal_Descriptor ins_desc{ gct_model::signal_Delivered_Insulin, dsGCT_Model_v1_Delivered_Insulin, dsU, scgms::NSignal_Unit::U_insulin, 0xFF450098, 0xFF450098, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
	const scgms::TSignal_Descriptor iob_desc{ gct_model::signal_IOB, dsGCT_Model_v1_IOB, dsU, scgms::NSignal_Unit::U_insulin, 0xFF456898, 0xFF456898, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
	const scgms::TSignal_Descriptor cob_desc{ gct_model::signal_COB, dsGCT_Model_v1_COB, dsU, scgms::NSignal_Unit::g, 0xFF45CC98, 0xFF45CC98, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 0.1 };
}

namespace gct2_model {

	const wchar_t* model_param_ui_names[model_param_count] = {
		// TODO: move to dstrings
		L"Q1_0", L"Q2_0", L"Qsc_0", L"I_0", L"Isc_0", L"X_0", L"D1_0",
		L"Vq", L"Vqsc", L"Vi", L"Q1b", L"Gthr", L"GIthr",
		L"q12", L"q1sc", L"ix", L"xq1", L"iscimod",
		L"q1e", L"q1ee", L"q1e_thr", L"xe",
		L"q1p", L"q1pe", L"q1pi", L"ip",
		L"e_pa", L"e_ua", L"e_pe", L"e_ue", L"q_ep", L"q_eu",
		L"e_lta", L"e_lte", L"e_Si",
		L"Ag", L"t_d", L"t_i",
		L"t_id",
	};

	const scgms::NModel_Parameter_Value model_param_types[model_param_count] = {
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptTime
	};

	constexpr size_t number_of_calculated_signals = 5;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		gct2_model::signal_IG,
		gct2_model::signal_BG,
		gct2_model::signal_Delivered_Insulin,
		gct2_model::signal_IOB,
		gct2_model::signal_COB,
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		dsGCT_Model_v2_IG,
		dsGCT_Model_v2_BG,
		dsGCT_Model_v2_Delivered_Insulin,
		dsGCT_Model_v2_IOB,
		dsGCT_Model_v2_COB,
	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		scgms::signal_IG,
		scgms::signal_BG,
		scgms::signal_Delivered_Insulin_Total,
		scgms::signal_IOB,
		scgms::signal_COB,
	};

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		dsGCT_Model_v2,
		nullptr,
		model_param_count,
		7,
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

	const scgms::TSignal_Descriptor ig_desc{ gct2_model::signal_IG, dsGCT_Model_v2_IG, dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF00A0FF, 0xFF00A0FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
	const scgms::TSignal_Descriptor bg_desc{ gct2_model::signal_BG, dsGCT_Model_v2_BG, dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0088, 0xFFFF0088, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
	const scgms::TSignal_Descriptor ins_desc{ gct2_model::signal_Delivered_Insulin, dsGCT_Model_v2_Delivered_Insulin, dsU, scgms::NSignal_Unit::U_insulin, 0xFF450098, 0xFF450098, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
	const scgms::TSignal_Descriptor iob_desc{ gct2_model::signal_IOB, dsGCT_Model_v2_IOB, dsU, scgms::NSignal_Unit::U_insulin, 0xFF456898, 0xFF456898, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
	const scgms::TSignal_Descriptor cob_desc{ gct2_model::signal_COB, dsGCT_Model_v2_COB, dsU, scgms::NSignal_Unit::g, 0xFF45CC98, 0xFF45CC98, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 0.1 };
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
		1,
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
		0,
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
		0,
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

namespace samadi_model { // DOI: 10.1016/j.compchemeng.2019.106565

	const wchar_t* model_param_ui_names[model_param_count] = {
		// TODO: move to dstrings
		L"Q1_0", L"Q2_0", L"Gsub_0", L"S1_0", L"S2_0", L"I_0", L"x1_0", L"x2_0", L"x3_0",
		L"D1_0", L"D2_0", L"DH1_0", L"DH2_0", L"E1_0", L"E2_0", L"TE_0",
		L"k12", L"ka1", L"ka2", L"ka3",
		L"kb1", L"kb2", L"kb3",
		L"ke",
		L"Vi", L"Vg",
		L"EGP_0", L"F01",
		L"tmaxi", L"tau_g", L"a", L"t_HR", L"t_in", L"n", L"t_ex", L"c1", L"c2",
		L"Ag",
		L"tmaxG",
		L"alpha", L"beta",
		L"BW", L"HRbase",
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
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble
	};

	constexpr size_t number_of_calculated_signals = 5;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		samadi_model::signal_IG,
		samadi_model::signal_BG,
		samadi_model::signal_Delivered_Insulin,
		samadi_model::signal_IOB,
		samadi_model::signal_COB,
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		L"Samadi model - IG",
		L"Samadi model - BG",
		L"Samadi model - Delivered insulin",
		L"Samadi model - IOB",
		L"Samadi model - COB",
	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		scgms::signal_IG,
		scgms::signal_BG,
		scgms::signal_Delivered_Insulin_Total,
		scgms::signal_IOB,
		scgms::signal_COB,
	};

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		L"Samadi model",
		nullptr,
		model_param_count,
		0,
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

	const std::wstring ist_desc = std::wstring{ L"Samadi model - IG" };
	const scgms::TSignal_Descriptor ig_desc{ samadi_model::signal_IG, ist_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring bgs_desc = std::wstring{ L"Samadi model - BG" };
	const scgms::TSignal_Descriptor bg_desc{ samadi_model::signal_BG, bgs_desc.c_str(), dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0088, 0xFFFF0088, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring inss_desc = std::wstring{ L"Samadi model - Delivered insulin" };
	const scgms::TSignal_Descriptor ins_desc{ samadi_model::signal_Delivered_Insulin, inss_desc.c_str(), dsU, scgms::NSignal_Unit::U_insulin, 0xFF450098, 0xFF450098, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring iobs_desc = std::wstring{ L"Samadi model - IOB" };
	const scgms::TSignal_Descriptor iob_desc{ samadi_model::signal_IOB, iobs_desc.c_str(), dsU, scgms::NSignal_Unit::U_insulin, 0xFF456898, 0xFF456898, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };

	const std::wstring cobs_desc = std::wstring{ L"Samadi model - COB" };
	const scgms::TSignal_Descriptor cob_desc{ samadi_model::signal_COB, cobs_desc.c_str(), dsU, scgms::NSignal_Unit::g, 0xFF45CC98, 0xFF45CC98, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 0.1 };
}

namespace p559_model {
	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble };

	const wchar_t* param_names[param_count] = { L"IG_0", L"Omega shift"};
	const wchar_t* param_columns[param_count] = { L"IG_0", L"Omega_shift"};

	const size_t signal_count = 1;

	const GUID signal_ids[signal_count] = { p559_model::ig_signal_id };
	const wchar_t* signal_names[signal_count] = { L"p559 IG" };
	const GUID reference_signal_ids[signal_count] = { scgms::signal_IG };

	const scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		L"p559 IG prediction",
		L"p559_ig_prediction",
		param_count,
		1, // TODO: fix scgms to not crash, when this equals 0
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


	const scgms::TSignal_Descriptor ig_desc{ p559_model::ig_signal_id, L"p559 IG", L"", scgms::NSignal_Unit::Other, 0xFF00CCCC, 0xFF00CCCC, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0};
}

namespace aim_ge {

	const wchar_t* model_param_ui_names[param_count] = {
		L"IG_0",
		L"R",L"R",L"R",L"R",L"R",L"R",L"R",L"R",L"R"
	};

	const scgms::NModel_Parameter_Value model_param_types[param_count] = {
		scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
	};

	constexpr size_t number_of_calculated_signals = 1;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		ig_id,
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		L"AIM GE - IG"
	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		scgms::signal_IG
	};

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		L"AIM GE",
		nullptr,
		param_count,
		param_count,
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

	const scgms::TSignal_Descriptor ig_desc{ ig_id, L"AIM GE - IBR", dsU_per_Hr, scgms::NSignal_Unit::U_per_Hr, 0xFF00FFFF, 0x0000FFFF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
}

namespace data_enhacement {

	const wchar_t* model_param_ui_names[param_count] = {
		L"t_det",L"t_delay",L"pf", L"nf",L"pf_shift",L"nf_shift"
	};

	const scgms::NModel_Parameter_Value model_param_types[param_count] = {
		scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptTime,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,
	};

	constexpr size_t number_of_calculated_signals = 2;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		pos_signal_id,
		neg_signal_id
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		L"DataEcmt - positive event",
		L"DataEcmt - negative event",
	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		scgms::signal_Carb_Intake,
		scgms::signal_Delivered_Insulin_Total,
	};

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		L"Data enhacement",
		nullptr,
		param_count,
		param_count,
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

	const scgms::TSignal_Descriptor pos_desc{ pos_signal_id, L"DataEcmt - positive event", L"g", scgms::NSignal_Unit::g, 0xFF00AA00, 0xFF00AA00, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::diamond, nullptr, 1.0};
	const scgms::TSignal_Descriptor neg_desc{ neg_signal_id, L"DataEcmt - negative event", dsU, scgms::NSignal_Unit::U_insulin, 0xFFAA0000, 0xFFAA0000, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::diamond, nullptr, 1.0 };
}

namespace cgp_pred {

	/*const wchar_t* model_param_ui_names[param_count] = {
		L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",
		L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",
		L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",
		L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",
		L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",L"c",
		L"c",
	};*/

	const std::array<const wchar_t*, param_count> model_param_ui_names{ generateArray<const wchar_t*, param_count>(L"c") };

	/*const scgms::NModel_Parameter_Value model_param_types[param_count] = {
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

		scgms::NModel_Parameter_Value::mptDouble
	};*/

	const std::array<scgms::NModel_Parameter_Value, param_count> model_param_types{ generateArray<scgms::NModel_Parameter_Value, param_count>(scgms::NModel_Parameter_Value::mptDouble) };

	constexpr size_t number_of_calculated_signals = NumOfForecastValues;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		{ 0x709DAAA9, 0x3F78, 0x4237, { 0x89, 0xFE, 0x4A, 0xF4, 0x90, 0x01, 0x91, 0x2C}}, // {709DAAA9-3F78-4237-89FE-4AF49001912C}
		{ 0x035B26CF, 0xB140, 0x4E24, { 0x87, 0xD4, 0xAB, 0xBF, 0x5C, 0xB3, 0x56, 0xED}}, // {035B26CF-B140-4E24-87D4-ABBF5CB356ED}
		{ 0x053EEF8F, 0x893C, 0x464D, { 0xB5, 0xCB, 0xD9, 0xBC, 0x1C, 0xFB, 0x26, 0xC3}}, // {053EEF8F-893C-464D-B5CB-D9BC1CFB26C3}
		{ 0x8FE9D427, 0x4642, 0x46EF, { 0xBB, 0x06, 0x51, 0xD5, 0x8A, 0xA8, 0xAB, 0x61}}, // {8FE9D427-4642-46EF-BB06-51D58AA8AB61}
		/*{ 0x8B4431A5, 0x7C79, 0x414C, { 0x9A, 0x5A, 0xC3, 0x71, 0x31, 0xAC, 0x7A, 0xD2}}, // {8B4431A5-7C79-414C-9A5A-C37131AC7AD2}
		{ 0x5B4DFDB8, 0x4EB9, 0x4EF4, { 0x99, 0x11, 0xE8, 0x8E, 0xD8, 0x98, 0x4F, 0xD0}}, // {5B4DFDB8-4EB9-4EF4-9911-E88ED8984FD0}
		{ 0xE428A828, 0x60A5, 0x420E, { 0xA3, 0x96, 0x7D, 0xEC, 0x58, 0x65, 0xF6, 0x03}}, // {E428A828-60A5-420E-A396-7DEC5865F603}
		{ 0x49A5C5FF, 0x730E, 0x41AD, { 0xB6, 0xEC, 0x14, 0xF9, 0x60, 0xB8, 0x8E, 0x5A}}, // {49A5C5FF-730E-41AD-B6EC-14F960B88E5A}
		{ 0xF1425618, 0x2A6E, 0x4730, { 0x81, 0x01, 0xD3, 0x45, 0xDC, 0x4D, 0xA6, 0xAB}}, // {F1425618-2A6E-4730-8101-D345DC4DA6AB}
		{ 0x83E3315F, 0xC89C, 0x4D14, { 0xB5, 0xB9, 0x07, 0xAC, 0xAB, 0x0B, 0xD5, 0xC5}}, // {83E3315F-C89C-4D14-B5B9-07ACAB0BD5C5}
		{ 0x6BC7E093, 0x349F, 0x4FB0, { 0xBF, 0x16, 0xB2, 0x8E, 0x34, 0x6B, 0x87, 0x6C}}, // {6BC7E093-349F-4FB0-BF16-B28E346B876C}
		{ 0xB98AF750, 0xA791, 0x40DC, { 0x97, 0x64, 0xE1, 0xA3, 0x41, 0x6B, 0xBF, 0x1B}}, // {B98AF750-A791-40DC-9764-E1A3416BBF1B}
		{ 0x4450276C, 0x19E4, 0x4577, { 0x9E, 0x47, 0x29, 0xEE, 0x65, 0xE3, 0x91, 0xBF}}, // {4450276C-19E4-4577-9E47-29EE65E391BF}
		{ 0x31AEE976, 0x8526, 0x44A5, { 0xA2, 0x15, 0xD1, 0xCD, 0xA5, 0x99, 0xAE, 0x76}}, // {31AEE976-8526-44A5-A215-D1CDA599AE76}
		{ 0x8053682D, 0x1A34, 0x4CBF, { 0xB6, 0x78, 0x6C, 0x2C, 0xBC, 0xF0, 0xE2, 0xBB}}, // {8053682D-1A34-4CBF-B678-6C2CBCF0E2BB}
		{ 0x16679B94, 0x28CB, 0x4DCB, { 0x82, 0xC1, 0x4F, 0x65, 0x77, 0x1A, 0x9F, 0xF1}}, // {16679B94-28CB-4DCB-82C1-4F65771A9FF1}
		{ 0x93131535, 0x762E, 0x41B5, { 0x85, 0x40, 0x48, 0xDC, 0x04, 0x50, 0x7B, 0x90}}, // {93131535-762E-41B5-8540-48DC04507B90}
		{ 0xE68B37DF, 0x3EA8, 0x4539, { 0x9D, 0xBF, 0x9D, 0xC8, 0x72, 0x1E, 0x2D, 0xB2}}, // {E68B37DF-3EA8-4539-9DBF-9DC8721E2DB2}
		{ 0x4131AD76, 0x7154, 0x415A, { 0x82, 0x59, 0x4B, 0x39, 0xAF, 0x57, 0x71, 0x6D}}, // {4131AD76-7154-415A-8259-4B39AF57716D}
		{ 0x11D70AF9, 0x27A2, 0x4BDC, { 0xBC, 0x85, 0x1B, 0x2F, 0x8D, 0x5D, 0x5C, 0x6A}}, // {11D70AF9-27A2-4BDC-BC85-1B2F8D5D5C6A}
		{ 0xFB8453A5, 0x4EF9, 0x4597, { 0x91, 0x33, 0x4C, 0x7C, 0xBF, 0x79, 0x2E, 0x1B}}, // {FB8453A5-4EF9-4597-9133-4C7CBF792E1B}
		{ 0x88EA33E4, 0x9FB1, 0x49C5, { 0xBC, 0xC3, 0x1E, 0xE7, 0x33, 0x7E, 0xF9, 0x7B}}, // {88EA33E4-9FB1-49C5-BCC3-1EE7337EF97B}
		{ 0xE568F2EF, 0x4C7D, 0x4D9A, { 0x91, 0x58, 0x31, 0x3C, 0x1A, 0x47, 0x95, 0xC3}}, // {E568F2EF-4C7D-4D9A-9158-313C1A4795C3}
		{ 0x9141E5FA, 0xD8E4, 0x4C95, { 0x88, 0xC4, 0x28, 0xFC, 0x81, 0x4A, 0x36, 0xD5}},*/ // {9141E5FA-D8E4-4C95-88C4-28FC814A36D5}
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		L"CGPPred - y1",L"CGPPred - y2",L"CGPPred - y3",L"CGPPred - y4",/*L"CGPPred - y5",L"CGPPred - y6",L"CGPPred - y7",L"CGPPred - y8",
		L"CGPPred - y9",L"CGPPred - y10",L"CGPPred - y11",L"CGPPred - y12",L"CGPPred - y13",L"CGPPred - y14",L"CGPPred - y15",L"CGPPred - y16",
		L"CGPPred - y17",L"CGPPred - y18",L"CGPPred - y19",L"CGPPred - y20",L"CGPPred - y21",L"CGPPred - y22",L"CGPPred - y23",L"CGPPred - y24",*/
	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,/*scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,
		scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,
		scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,scgms::signal_IG,*/
	};

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		L"CGP Prediction",
		nullptr,
		param_count,
		param_count,
		model_param_types.data(),
		const_cast<const wchar_t**>(model_param_ui_names.data()),
		nullptr,
		lower_bounds.data(),
		default_parameters.data(),
		upper_bounds.data(),

		number_of_calculated_signals,
		calculated_signal_ids,
		reference_signal_ids,
	};

	const uint32_t signalColors[4] = { 0xFF66FF33, 0xFFFF6633, 0xFF3366FF, 0xFF33FF66 };

	const scgms::TSignal_Descriptor Describe_Pred_Signal(size_t index)
	{
		return scgms::TSignal_Descriptor{ calculated_signal_ids[index], calculated_signal_names[index], dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, signalColors[index], signalColors[index], scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0};
	}

	const scgms::TSignal_Descriptor signal_descs[number_of_calculated_signals] = {
		Describe_Pred_Signal(0),Describe_Pred_Signal(1),Describe_Pred_Signal(2),Describe_Pred_Signal(3),/*Describe_Pred_Signal(4),Describe_Pred_Signal(5),Describe_Pred_Signal(6),Describe_Pred_Signal(7),
		Describe_Pred_Signal(8),Describe_Pred_Signal(9),Describe_Pred_Signal(10),Describe_Pred_Signal(11),Describe_Pred_Signal(12),Describe_Pred_Signal(13),Describe_Pred_Signal(14),Describe_Pred_Signal(15),
		Describe_Pred_Signal(16),Describe_Pred_Signal(17),Describe_Pred_Signal(18),Describe_Pred_Signal(19),Describe_Pred_Signal(20),Describe_Pred_Signal(21),Describe_Pred_Signal(22),Describe_Pred_Signal(23),
	*/};
}

namespace bases_pred {

	const wchar_t* model_param_ui_names[param_count] = {
		L"W_g", L"b_win", L"b_off", L"Cc", L"Ic", L"PAc", L"c_hist", L"i_hist",
		
		L"b1_a", L"b1_toff", L"b1_var",
		L"b2_a", L"b2_toff", L"b2_var",
		L"b3_a", L"b3_toff", L"b3_var",
		L"b4_a", L"b4_toff", L"b4_var",
		L"b5_a", L"b5_toff", L"b5_var",
		L"b6_a", L"b6_toff", L"b6_var",
		L"b7_a", L"b7_toff", L"b7_var",
		L"b8_a", L"b8_toff", L"b8_var",
		L"b9_a", L"b9_toff", L"b9_var",

		L"c", L"k", L"h",
	};

	const scgms::NModel_Parameter_Value model_param_types[param_count] = {
		scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptTime, scgms::NModel_Parameter_Value::mptTime,

		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,

		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,
	};

	constexpr size_t number_of_calculated_signals = 1;

	const GUID calculated_signal_ids[number_of_calculated_signals] = {
		ig_signal_id,
	};

	const wchar_t* calculated_signal_names[number_of_calculated_signals] = {
		L"BasesPred - IG",
	};

	const GUID reference_signal_ids[number_of_calculated_signals] = {
		scgms::signal_IG,
	};

	scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		L"BasesPred",
		nullptr,
		param_count,
		param_count,
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

	const scgms::TSignal_Descriptor ig_desc{ ig_signal_id, L"BasesPred - IG", dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFff8c00, 0xFFff8c00, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr, 1.0 };
}


const std::array<const scgms::TFilter_Descriptor, 1> filter_descriptions = { { pattern_prediction::get_filter_desc()} };

const std::array<scgms::TModel_Descriptor, 19> model_descriptions = { { diffusion_v2_model::desc,
																		 steil_rebrin::desc, steil_rebrin_diffusion_prediction::desc, diffusion_prediction::desc,
																		 constant_model::desc,
																		 bergman_model::desc,
																		 uva_padova_S2013::desc,
																		 uva_padova_S2017::desc,
																		 insulin_bolus::desc,
																		 const_isf::desc, const_cr::desc,
																		 samadi_model::desc,
																		 gct_model::desc,
																		 gct2_model::desc,
																		 p559_model::desc,
																		 aim_ge::desc,
																		 data_enhacement::desc,
																		 cgp_pred::desc,
																		 bases_pred::desc,
																		} };

const std::array<scgms::TSignal_Descriptor, 41+cgp_pred::number_of_calculated_signals> signals_descriptors = { {diffusion_v2_model::bg_desc, diffusion_v2_model::ig_desc, steil_rebrin::bg_desc,
																		 steil_rebrin_diffusion_prediction::ig_desc, diffusion_prediction::ig_desc, 
																		 constant_model::const_desc,
																		 bergman_model::bg_desc, bergman_model::ig_desc, bergman_model::iob_desc, bergman_model::cob_desc, bergman_model::basal_insulin_desc, bergman_model::insulin_activity_desc,
																		 uva_padova_S2013::ig_desc, uva_padova_S2013::bg_desc, uva_padova_S2013::ins_desc,
																		 uva_padova_S2017::ig_desc, uva_padova_S2017::bg_desc, uva_padova_S2017::ins_desc, uva_padova_S2017::iob_desc, uva_padova_S2017::cob_desc,
																		 samadi_model::ig_desc, samadi_model::bg_desc, samadi_model::ins_desc, samadi_model::iob_desc, samadi_model::cob_desc,
																		 gct_model::ig_desc, gct_model::bg_desc, gct_model::ins_desc, gct_model::iob_desc, gct_model::cob_desc,
																		 gct2_model::ig_desc, gct2_model::bg_desc, gct2_model::ins_desc, gct2_model::iob_desc, gct2_model::cob_desc,
																		 pattern_prediction::get_sig_desc(),
																		 p559_model::ig_desc,
																		 aim_ge::ig_desc,
																		 data_enhacement::pos_desc, data_enhacement::neg_desc,
	bases_pred::ig_desc,

	cgp_pred::signal_descs[0],cgp_pred::signal_descs[1],cgp_pred::signal_descs[2],cgp_pred::signal_descs[3],/*cgp_pred::signal_descs[4],cgp_pred::signal_descs[5],cgp_pred::signal_descs[6],cgp_pred::signal_descs[7],
	cgp_pred::signal_descs[8],cgp_pred::signal_descs[9],cgp_pred::signal_descs[10],cgp_pred::signal_descs[11],cgp_pred::signal_descs[12],cgp_pred::signal_descs[13],cgp_pred::signal_descs[14],cgp_pred::signal_descs[15],
	cgp_pred::signal_descs[16],cgp_pred::signal_descs[17],cgp_pred::signal_descs[18],cgp_pred::signal_descs[19],cgp_pred::signal_descs[20],cgp_pred::signal_descs[21],cgp_pred::signal_descs[22],cgp_pred::signal_descs[23],
*/																		}};

HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end) {
	*begin = const_cast<scgms::TModel_Descriptor*>(model_descriptions.data());
	*end = *begin + model_descriptions.size();
	return S_OK;
}

HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor * *begin, scgms::TSignal_Descriptor * *end) {
	*begin = const_cast<scgms::TSignal_Descriptor*>(signals_descriptors.data());
	*end = *begin + signals_descriptors.size();
	return S_OK;
}

HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model) {
	if (*model_id == bergman_model::model_id) return Manufacture_Object<CBergman_Discrete_Model>(model, parameters, output);
	else if (*model_id == uva_padova_S2013::model_id) return Manufacture_Object<CUVA_Padova_S2013_Discrete_Model>(model, parameters, output);
	else if (*model_id == uva_padova_S2017::model_id) return Manufacture_Object<CUVA_Padova_S2017_Discrete_Model>(model, parameters, output);
	else if (*model_id == insulin_bolus::model_id) return Manufacture_Object<CDiscrete_Insulin_Bolus_Calculator>(model, parameters, output);
	else if (*model_id == samadi_model::model_id) return Manufacture_Object<CSamadi_Discrete_Model>(model, parameters, output);
	else if (*model_id == gct_model::model_id) return Manufacture_Object<CGCT_Discrete_Model>(model, parameters, output);
	else if (*model_id == gct2_model::model_id) return Manufacture_Object<CGCT2_Discrete_Model>(model, parameters, output);
	else if (*model_id == p559_model::model_id) return Manufacture_Object<CP559_Model>(model, parameters, output);
	else if (*model_id == aim_ge::model_id) return Manufacture_Object<CAIM_GE_Model>(model, parameters, output);
	else if (*model_id == data_enhacement::model_id) return Manufacture_Object<CData_Enhacement_Model>(model, parameters, output);
	else if (*model_id == cgp_pred::model_id) return Manufacture_Object<CCGP_Prediction>(model, parameters, output);
	else if (*model_id == bases_pred::model_id) return Manufacture_Object<CBase_Functions_Predictor>(model, parameters, output);
		else return E_NOTIMPL;
}

HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {
	*begin = const_cast<scgms::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter) {
	if (*id == pattern_prediction::filter_id)
		return Manufacture_Object<CPattern_Prediction_Filter>(filter, output);

	return E_NOTIMPL;
}
