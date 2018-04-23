#pragma once

#include "descriptor.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/descriptor_utils.h"
#include "../../../common/rtl/manufactory.h"
#include "Diffusion_v2_blood.h"
#include "Diffusion_v2_ist.h"
#include "Steil_Rebrin_blood.h"
#include <vector>

namespace diffusion_v2_model {
	const GUID id = { 0x6645466a, 0x28d6, 0x4536,{ 0x9a, 0x38, 0xf, 0xd6, 0xea, 0x6f, 0xdb, 0x2d } }; // {6645466A-28D6-4536-9A38-0FD6EA6FDB2D}
	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble , glucose::NModel_Parameter_Value::mptDouble , glucose::NModel_Parameter_Value::mptTime, glucose::NModel_Parameter_Value::mptDouble , glucose::NModel_Parameter_Value::mptTime };
	const wchar_t *param_names[param_count] = {dsP, dsCg, dsC, dsDt, dsK, dsH};
	const wchar_t *param_columns[param_count] = { rsP_Column, rsCg_Column, rsC_Column, rsDt_Column, rsK_Column, rsH_Column };

	const double lower_bound[param_count] = {0.0, -0.5, -10.0, 0.0, -1.0, 0.0};
	const double upper_bound[param_count] = { 2.0, 0.0, 10.0, glucose::One_Hour, 0.0, glucose::One_Hour };

	const size_t signal_count = 2;
	
	const GUID signal_ids[signal_count] = { glucose::signal_Diffusion_v2_Blood, glucose::signal_Diffusion_v2_Ist };
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
	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble , glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble };
	const wchar_t *param_names[param_count] = { dsTau, dsAlpha, dsBeta, dsGamma };
	const wchar_t *param_columns[param_count] = { rsTau_Column, rsAlpha_Column, rsBeta_Column, rsGamma_Column };

	const double lower_bound[param_count] = { -1000.0, -1000.0, -1000.0, -1000.0 };
	const double upper_bound[param_count] = { 1000.0, 1000.0, 1000.0, 1000.0 };

	const size_t signal_count = 1;	
	const GUID signal_ids[signal_count] = { glucose::signal_Steil_Rebrin_Blood };
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

const std::vector<glucose::TModel_Descriptor> model_descriptions = { diffusion_v2_model::desc, steil_rebrin::desc };


HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end) {
	return do_get_descriptors(model_descriptions, begin, end);
}