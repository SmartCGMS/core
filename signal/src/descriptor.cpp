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
#include "calculate.h"
#include "mapping.h"
#include "masking.h"
#include "Measured_Signal.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace calculate {

	constexpr size_t param_count = 20;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptNull,
		glucose::NParameter_Type::ptModel_Id,
		glucose::NParameter_Type::ptModel_Signal_Id,
		glucose::NParameter_Type::ptRatTime,
		glucose::NParameter_Type::ptNull,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptSolver_Id,
		glucose::NParameter_Type::ptModel_Bounds,
		glucose::NParameter_Type::ptInt64,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptNull,
		glucose::NParameter_Type::ptMetric_Id,
		glucose::NParameter_Type::ptInt64,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptDouble
	};

	const wchar_t* ui_param_name[param_count] = {
		dsCalculation,
		dsSelected_Model,
		dsSelected_Signal,
		dsPrediction_Window,
		dsSolving_Parameters_Separator,
		dsSolve_Parameters,
		dsSelected_Solver,
		dsSelected_Model_Bounds,
		dsSolve_On_Level_Count,
		dsSolve_On_Calibration,
		dsSolve_On_Time_Segment_End,
		dsSolve_Using_All_Segments,
		dsMetric_Separator,
		dsSelected_Metric,
		dsMetric_Levels_Required,
		dsUse_Measured_Levels,
		dsUse_Relative_Error,
		dsUse_Squared_Diff,
		dsUse_Prefer_More_Levels,
		dsMetric_Threshold
	};

	const wchar_t* config_param_name[param_count] = {
		nullptr,
		rsSelected_Model,
		rsSelected_Signal,
		rsPrediction_Window,
		nullptr,
		rsSolve_Parameters,
		rsSelected_Solver,
		rsSelected_Model_Bounds,
		rsSolve_On_Level_Count,
		rsSolve_On_Calibration,
		rsSolve_On_Time_Segment_End,
		rsSolve_Using_All_Segments,
		nullptr,
		rsSelected_Metric,
		rsMetric_Levels_Required,
		rsUse_Measured_Levels,
		rsUse_Relative_Error,
		rsUse_Squared_Diff,
		rsUse_Prefer_More_Levels,
		rsMetric_Threshold
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		dsSelected_Model_Tooltip,
		dsSelected_Signal_Tooltip,
		dsPrediction_Window_Tooltip,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		dsMetric_Levels_Required_Hint,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};

	const glucose::TFilter_Descriptor Calculate_Descriptor = {
		{ 0x14a25f4c, 0xe1b1, 0x85c4,{ 0x12, 0x74, 0x9a, 0x0d, 0x11, 0xe0, 0x98, 0x13 } },  // {14A25F4C-E1B1-85C4-1274-9A0D11E09813}
		dsCalculate_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

namespace mapping
{
	constexpr size_t param_count = 2;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptSignal_Id,
		glucose::NParameter_Type::ptSignal_Id
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSignal_Source_Id,
		dsSignal_Destination_Id
	};

	const wchar_t* config_param_name[param_count] = {
		rsSignal_Source_Id,
		rsSignal_Destination_Id
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsMapping_Source_Signal_Tooltip,
		dsMapping_Destination_Signal_Tooltip
	};

	const glucose::TFilter_Descriptor Mapping_Descriptor = {
		{ 0x8fab525c, 0x5e86, 0xab81,{ 0x12, 0xcb, 0xd9, 0x5b, 0x15, 0x88, 0x53, 0x0A } }, //// {8FAB525C-5E86-AB81-12CB-D95B1588530A}
		dsMapping_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

namespace masking
{
	constexpr size_t param_count = 2;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptSignal_Id,
		glucose::NParameter_Type::ptWChar_Container // TODO: some type for bitmask?
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSignal_Masked_Id,
		dsSignal_Value_Bitmask
	};

	const wchar_t* config_param_name[param_count] = {
		rsSignal_Masked_Id,
		rsSignal_Value_Bitmask
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsMasked_Signal_Tooltip,
		dsSignal_Values_Mask_Tooltip
	};

	const glucose::TFilter_Descriptor Masking_Descriptor = {
		{ 0xa1124c89, 0x18a4, 0xf4c1,{ 0x28, 0xe8, 0xa9, 0x47, 0x1a, 0x58, 0x02, 0x1e } }, //// {A1124C89-18A4-F4C1-28E8-A9471A58021Q}
		dsMasking_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

namespace measured_signal
{
	constexpr size_t supported_count = 7;

	const GUID supported_signal_ids[supported_count] = {
		glucose::signal_IG,
		glucose::signal_BG,
		glucose::signal_ISIG,
		glucose::signal_Insulin,
		glucose::signal_Carb_Intake,
		glucose::signal_Calibration,
		glucose::signal_Health_Stress
	};
}
const std::array<glucose::TFilter_Descriptor, 3> filter_descriptions = { { calculate::Calculate_Descriptor, mapping::Mapping_Descriptor, masking::Masking_Descriptor } };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == calculate::Calculate_Descriptor.id)
		return Manufacture_Object<CCalculate_Filter>(filter, input, output);
	else if (*id == mapping::Mapping_Descriptor.id)
		return Manufacture_Object<CMapping_Filter>(filter, input, output);
	else if (*id == masking::Masking_Descriptor.id)
		return Manufacture_Object<CMasking_Filter>(filter, input, output);

	return ENOENT;
}

extern "C" HRESULT IfaceCalling do_create_signal(const GUID *signal_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) {
	if ((signal_id == nullptr) || (segment == nullptr))
		return E_INVALIDARG;

	for (size_t i = 0; i < measured_signal::supported_count; i++)
	{
		if (measured_signal::supported_signal_ids[i] == *signal_id)
			return Manufacture_Object<CMeasured_Signal>(signal);
	}

	return E_FAIL;
}
