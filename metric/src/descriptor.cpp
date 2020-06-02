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

#include "signal_error.h"
#include "signal_stats.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/descriptor_utils.h"

const std::array < scgms::TMetric_Descriptor, 13 > metric_descriptor = { {
	 scgms::TMetric_Descriptor{ mtrAvg_Abs, dsAvg_Abs },
	 scgms::TMetric_Descriptor{ mtrMax_Abs, dsMax_Abs },
	 scgms::TMetric_Descriptor{ mtrPerc_Abs, dsPerc_Abs },
	 scgms::TMetric_Descriptor{ mtrThresh_Abs, dsThresh_Abs },
	 scgms::TMetric_Descriptor{ mtrLeal_2010, dsLeal_2010 },
	 scgms::TMetric_Descriptor{ mtrAIC, dsAIC },
	 scgms::TMetric_Descriptor{ mtrStd_Dev, dsBessel_Std_Dev },
	 scgms::TMetric_Descriptor{ mtrVariance, dsBessel_Variance },
	 scgms::TMetric_Descriptor{ mtrCrosswalk, dsCrosswalk },
	 scgms::TMetric_Descriptor{ mtrIntegral_CDF, dsIntegral_CDF },
	 scgms::TMetric_Descriptor{ mtrAvg_Plus_Bessel_Std_Dev, dsAvg_Plus_Bessel_Std_Dev },
	 scgms::TMetric_Descriptor{ mtrRMSE, dsRMSE },
	 scgms::TMetric_Descriptor{ mtrExpWtDiff, dsExpWtDiffPolar },
} };

HRESULT IfaceCalling do_get_metric_descriptors(scgms::TMetric_Descriptor const **begin, scgms::TMetric_Descriptor const **end) {
	*begin = metric_descriptor.data();
	*end = *begin + metric_descriptor.size();
	return S_OK;
}

namespace signal_error {

	constexpr size_t param_count = 12;

	const scgms::NParameter_Type parameter_type[param_count] = {
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptMetric_Id,
		scgms::NParameter_Type::ptInt64,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptDouble,
		scgms::NParameter_Type::ptNull,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
	};

	const wchar_t* ui_parameter_name[param_count] = {
		dsDescription,
		dsReference_Signal,
		dsError_Signal,
		dsSelected_Metric,
		dsMetric_Levels_Required,
		dsUse_Relative_Error,
		dsUse_Squared_Diff,
		dsUse_Prefer_More_Levels,
		dsMetric_Threshold,
		nullptr,
		dsEmit_metric_as_signal,
		dsEmit_last_value_only
	};

	const wchar_t* config_parameter_name[param_count] = {
		rsDescription,
		rsReference_Signal,
		rsError_Signal,
		rsSelected_Metric,
		rsMetric_Levels_Required,
		rsUse_Relative_Error,
		rsUse_Squared_Diff,
		rsUse_Prefer_More_Levels,
		rsMetric_Threshold,
		nullptr,
		rsEmit_metric_as_signal,
		rsEmit_last_value_only
	};

	const wchar_t* ui_parameter_tooltip[param_count] = {
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		dsMetric_Levels_Required_Hint,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};

	const scgms::TFilter_Descriptor desc = {
		{ 0x690fbc95, 0x84ca, 0x4627, { 0xb4, 0x7c, 0x99, 0x55, 0xea, 0x81, 0x7a, 0x4f } },
		scgms::NFilter_Flags::None,
		dsSignal_Error,
		param_count,
		parameter_type,
		ui_parameter_name,
		config_parameter_name,
		ui_parameter_tooltip
	};


	const scgms::TSignal_Descriptor signal_desc {
		metric_signal_id, dsSignal_GUI_Name_Error_Metric, L"", scgms::NSignal_Unit::Other, 0xFF000000, 0xFF000000, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::none, nullptr};
}


namespace signal_stats {
	constexpr size_t param_count = 3;


	const scgms::NParameter_Type parameter_type[param_count] = {
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptBool
	};

	const wchar_t* ui_parameter_name[param_count] = {
		dsSelected_Signal,
		dsOutput_CSV_File,
		dsDiscard_Repeating_Level
	};

	const wchar_t* config_parameter_name[param_count] = {
		rsSelected_Signal,
		rsOutput_CSV_File,
		rsDiscard_Repeating_Level
	};

	const wchar_t* ui_parameter_tooltip[param_count] = {
		nullptr,
		nullptr,
		nullptr
	};

	const scgms::TFilter_Descriptor desc = {
		{ 0x6b405b74, 0x27d9, 0x49d7, { 0xb9, 0xff, 0x4, 0xda, 0x72, 0xf, 0x7, 0x7d } },
		scgms::NFilter_Flags::None,
		dsSignal_Stats,
		param_count,
		parameter_type,
		ui_parameter_name,
		config_parameter_name,
		ui_parameter_tooltip
	};
}


static const std::array<scgms::TFilter_Descriptor, 2> filter_descriptions = { signal_error::desc, signal_stats::desc  };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(const scgms::TFilter_Descriptor* *begin, scgms::TFilter_Descriptor const **end) {
	*begin = const_cast<scgms::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor const ** begin, scgms::TSignal_Descriptor const **end) {
	*begin = &signal_error::signal_desc;
	*end = *begin + 1;
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter) {
	if (*id == signal_error::desc.id) return Manufacture_Object<CSignal_Error>(filter, output);
		else if (*id == signal_stats::desc.id) return Manufacture_Object<CSignal_Stats>(filter, output);

	return ENOENT;
}
