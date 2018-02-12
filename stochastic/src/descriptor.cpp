#pragma once

#include "descriptor.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/descriptor_utils.h"
#include <vector>

namespace diffusion_v2_model {
	const GUID id = { 0x6645466a, 0x28d6, 0x4536,{ 0x9a, 0x38, 0xf, 0xd6, 0xea, 0x6f, 0xdb, 0x2d } }; // {6645466A-28D6-4536-9A38-0FD6EA6FDB2D}
	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble , glucose::NModel_Parameter_Value::mptDouble , glucose::NModel_Parameter_Value::mptTime, glucose::NModel_Parameter_Value::mptDouble , glucose::NModel_Parameter_Value::mptTime };
	const wchar_t *param_names[param_count] = {dsP, dsCg, dsC, dsDt, dsK, dsH};

	const double lower_bound[param_count] = {0.0, -0.5, -10.0, 0.0, -1.0, 0.0};
	const double default_values[param_count] = { 1.091213, -0.015811, -0.174114, 0.0233101854166667, -2.6E-6, 0.0185995368055556 };
	const double upper_bound[param_count] = { 2.0, 0.0, 10.0, glucose::One_Hour, 0.0, glucose::One_Hour };

	const size_t signal_count = 2;
	
	const GUID signal_ids[signal_count] = { glucose::signal_Diffusion_v2_Blood, glucose::signal_Diffusion_v2_Ist };
	const wchar_t *signal_names[signal_count] = { dsBlood, dsInterstitial };

	glucose::TModel_Descriptor desc = {
		id,
		dsDiffusion_Model_v2,
		param_count,
		param_types,
		param_names,
		lower_bound,
		default_values,
		upper_bound,
		signal_count,
		signal_ids,
		signal_names
	};
		

}

namespace steil_rebrin {
	const GUID id = { 0x5fd93b14, 0xaaa9, 0x44d7,{ 0xa8, 0xb8, 0xc1, 0x58, 0x31, 0x83, 0x64, 0xbd } };  // {5FD93B14-AAA9-44D7-A8B8-C158318364BD}
	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble , glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble };
	const wchar_t *param_names[param_count] = { dsTau, dsAlpha, dsBeta, dsGamma};

	const double lower_bound[param_count] = { -1000.0, -1000.0, -1000.0, -1000.0 };
	const double default_values[param_count] = { 0.00576459, 1.0/ 1.02164072, 0.0, 0.0};
	const double upper_bound[param_count] = { 1000.0, 1000.0, 1000.0, 1000.0 };

	const size_t signal_count = 1;	
	const GUID signal_ids[signal_count] = { glucose::signal_Steil_Rebrin_Blood };
	const wchar_t *signal_names[signal_count] = { dsBlood};

	const glucose::TModel_Descriptor desc = {
		id,
		dsSteil_Rebrin,
		param_count,
		param_types,
		param_names,
		lower_bound,
		default_values,
		upper_bound,
		signal_count,
		signal_ids,
		signal_names
	};

}

namespace newuoa {
	const glucose::TSolver_Descriptor desc{
			id, 
			dsNewUOA,
			false,
			0,
			nullptr
	};
}


const std::vector<glucose::TModel_Descriptor> model_descriptions = { diffusion_v2_model::desc, steil_rebrin::desc };
const std::vector<glucose::TSolver_Descriptor> solver_descriptions = { newuoa::desc };



HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end) {
	return do_get_descriptors(model_descriptions, begin, end);
}


HRESULT IfaceCalling do_get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end) {
	return do_get_descriptors(solver_descriptions, begin, end);
}