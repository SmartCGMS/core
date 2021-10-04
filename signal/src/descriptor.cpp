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
#include "calculated_signal.h"
#include "mapping.h"
#include "decoupling.h"
#include "masking.h"
#include "unmasking.h"
#include "Measured_Signal.h"
#include "signal_generator.h"
#include "signal_feedback.h"
#include "impulse_response_filter.h"
#include "median_response_filter.h"
#include "noise_filter.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace calculate {

	constexpr size_t param_count = 20;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptNull,
		scgms::NParameter_Type::ptSignal_Model_Id,
		scgms::NParameter_Type::ptModel_Produced_Signal_Id,
		scgms::NParameter_Type::ptRatTime,
		scgms::NParameter_Type::ptNull,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptSolver_Id,
		scgms::NParameter_Type::ptDouble_Array,
		scgms::NParameter_Type::ptInt64,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptNull,
		scgms::NParameter_Type::ptMetric_Id,
		scgms::NParameter_Type::ptInt64,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptDouble
	};

	const wchar_t* ui_param_name[param_count] = {
		dsCalculated_Signal,
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

	const scgms::TFilter_Descriptor Calculate_Descriptor = {
		{ 0x14a25f4c, 0xe1b1, 0x85c4,{ 0x12, 0x74, 0x9a, 0x0d, 0x11, 0xe0, 0x98, 0x13 } },  // {14A25F4C-E1B1-85C4-1274-9A0D11E09813}
		scgms::NFilter_Flags::None,
		dsCalculated_Signal_Filter,
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

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id
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

	const scgms::TFilter_Descriptor Mapping_Descriptor = {
		{ 0x8fab525c, 0x5e86, 0xab81,{ 0x12, 0xcb, 0xd9, 0x5b, 0x15, 0x88, 0x53, 0x0A } }, //// {8FAB525C-5E86-AB81-12CB-D95B1588530A}
		scgms::NFilter_Flags::None,
		dsMapping_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}


namespace decoupling
{
	constexpr size_t param_count = 6;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptWChar_Array
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSignal_Source_Id,
		dsSignal_Destination_Id,
		dsRemove_From_Source,
		dsCondition,
		dsCollect_Statistics,
		dsOutput_CSV_File
	};

	const wchar_t* config_param_name[param_count] = {
		rsSignal_Source_Id,
		rsSignal_Destination_Id,
		rsRemove_From_Source,
		rsCondition,
		rsCollect_Statistics,
		rsOutput_CSV_File
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsMapping_Source_Signal_Tooltip,
		dsMapping_Destination_Signal_Tooltip,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};

	const scgms::TFilter_Descriptor desc = {
		{ 0xbb71190, 0x8709, 0x4990, { 0x97, 0xfa, 0x6a, 0x4d, 0xb6, 0x79, 0xef, 0x1d } },
		scgms::NFilter_Flags::None,
		dsDecoupling_Filter,
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

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptWChar_Array// TODO: some type for bitmask?
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

	const scgms::TFilter_Descriptor Masking_Descriptor = {
		{ 0xa1124c89, 0x18a4, 0xf4c1,{ 0x28, 0xe8, 0xa9, 0x47, 0x1a, 0x58, 0x02, 0x1e } }, //// {A1124C89-18A4-F4C1-28E8-A9471A58021E}
		scgms::NFilter_Flags::None,
		dsMasking_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}


namespace unmasking {
	constexpr size_t param_count = 1;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptSignal_Id,		
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSignal_Masked_Id,		
	};

	const wchar_t* config_param_name[param_count] = {
		rsSignal_Masked_Id,		
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsMasked_Signal_Tooltip,		
	};

	const scgms::TFilter_Descriptor Unmasking_Descriptor = {
		{ 0x5a883e4f, 0x198e, 0x4dde, { 0xa4, 0xfa, 0xbb, 0x33, 0x30, 0x1a, 0xdb, 0xd1 }}, // {5A883E4F-198E-4DDE-A4FA-BB33301ADBD1}
		scgms::NFilter_Flags::None,
		dsUnmasking_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

namespace signal_generator {

	constexpr size_t filter_param_count = 10;
	const wchar_t *filter_ui_names[filter_param_count] = {
		dsSelected_Model,
		dsFeedback_Name,
		dsSynchronize_to_Signal,
		dsSynchronization_Signal,
		dsTime_Segment_ID,
		dsStepping,
		dsMaximum_Time,
		dsShutdown_After_Last,
		dsEcho_Default_Parameters_As_Event,
		dsParameters
	};

	const wchar_t *filter_config_names[filter_param_count] = {
		rsSelected_Model,
		rsFeedback_Name,
		rsSynchronize_to_Signal,
		rsSynchronization_Signal,
		rsTime_Segment_ID,
		rsStepping,
		rsMaximum_Time,
		rsShutdown_After_Last,
		rsEcho_Default_Parameters_As_Event,
		rsParameters
	};

	const wchar_t *filter_tooltips[filter_param_count] = {
		dsSelected_Model_Tooltip,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};

	constexpr scgms::NParameter_Type filter_param_types[filter_param_count] = {
		scgms::NParameter_Type::ptDiscrete_Model_Id,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptInt64,
		scgms::NParameter_Type::ptRatTime,
		scgms::NParameter_Type::ptRatTime,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptDouble_Array
	};

	scgms::TFilter_Descriptor desc = {
		{ 0x9eeb3451, 0x2a9d, 0x49c1, { 0xba, 0x37, 0x2e, 0xc0, 0xb0, 0xe, 0x5e, 0x6d } },  // {9EEB3451-2A9D-49C1-BA37-2EC0B00E5E6D}
		scgms::NFilter_Flags::None,
		dsSignal_Generator,
		filter_param_count,
		filter_param_types,
		filter_ui_names,
		filter_config_names,
		filter_tooltips
	};
}

namespace network_signal_generator {

	constexpr size_t filter_param_count = 14;

	const wchar_t *filter_ui_names[filter_param_count] = {
		dsSelected_Model,
		dsFeedback_Name,
		dsSynchronize_to_Signal,
		dsSynchronization_Signal,
		dsTime_Segment_ID,
		dsStepping,
		dsMaximum_Time,
		dsShutdown_After_Last,
		dsEcho_Default_Parameters_As_Event,
		dsParameters,
		dsRemote_Host,
		dsRemote_Port,
		dsRemote_Model_Id,
		dsRemote_Subject_Name,
	};

	const wchar_t *filter_config_names[filter_param_count] = {
		rsSelected_Model,
		rsFeedback_Name,
		rsSynchronize_to_Signal,
		rsSynchronization_Signal,
		rsTime_Segment_ID,
		rsStepping,
		rsMaximum_Time,
		rsShutdown_After_Last,
		rsEcho_Default_Parameters_As_Event,
		rsParameters,
		rsRemote_Host,
		rsRemote_Port,
		rsRemote_Model_Id,
		rsRemote_Subject_Name,
	};

	const wchar_t *filter_tooltips[filter_param_count] = {
		dsSelected_Model_Tooltip,
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
		nullptr,
		nullptr,
		nullptr
	};

	constexpr scgms::NParameter_Type filter_param_types[filter_param_count] = {
		scgms::NParameter_Type::ptDiscrete_Model_Id,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptInt64,
		scgms::NParameter_Type::ptRatTime,
		scgms::NParameter_Type::ptRatTime,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptDouble_Array,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptInt64,
		scgms::NParameter_Type::ptSignal_Id,		// NOTE: this model will not be enumerated in GUI as its descriptor is missing
		scgms::NParameter_Type::ptWChar_Array,
	};

	scgms::TFilter_Descriptor desc = {
		{ 0x8a583293, 0x20a7, 0x4d56, { 0x93, 0x2f, 0xc5, 0x68, 0xb8, 0xe3, 0xd4, 0x4a } }, // {8A583293-20A7-4D56-932F-C568B8E3D44A}
		scgms::NFilter_Flags::None,
		dsSignal_Generator_Network,
		filter_param_count,
		filter_param_types,
		filter_ui_names,
		filter_config_names,
		filter_tooltips
	};
}

namespace signal_descriptor {
	const scgms::TSignal_Descriptor bg_desc { scgms::signal_BG, dsSignal_GUI_Name_BG, dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0000, 0xFFFF0000, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor bg_cal_desc{ scgms::signal_Calibration, dsSignal_GUI_Name_Calibration, dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFFFF0000, 0xFFFF0000, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor ig_desc { scgms::signal_IG, dsSignal_GUI_Name_IG, dsmmol_per_L, scgms::NSignal_Unit::mmol_per_L, 0xFF0000FF, 0xFF0000FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor isig_desc{ scgms::signal_ISIG, dsSignal_GUI_Name_ISIG, dsA, scgms::NSignal_Unit::A, 0xFF8080FF, 0xFF8080FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor req_bolus_desc{ scgms::signal_Requested_Insulin_Bolus, dsSignal_Requested_Insulin_Bolus, dsU, scgms::NSignal_Unit::U_insulin, 0xFFF59053, 0xFFF59053, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor req_ibr_desc{ scgms::signal_Requested_Insulin_Basal_Rate, dsSignal_Requested_Insulin_Basal_Rate, dsU_per_Hr, scgms::NSignal_Unit::U_per_Hr, 0xFFDDDD55, 0xFFDDDD55, scgms::NSignal_Visualization::step, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor req_iidr_desc{ scgms::signal_Requested_Insulin_Intradermal_Rate, dsSignal_Requested_Insulin_Intradermal_Rate, dsU_per_Hr, scgms::NSignal_Unit::U_per_Hr, 0xFFDD8855, 0xFFDD8855, scgms::NSignal_Visualization::step, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor del_bolus_desc{ scgms::signal_Delivered_Insulin_Bolus, dsSignal_Delivered_Insulin_Bolus, dsU, scgms::NSignal_Unit::U_insulin, 0xFFF59053, 0xFFF59053, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor del_ins_total_desc{ scgms::signal_Delivered_Insulin_Total, dsSignal_Delivered_Insulin_Total, dsU, scgms::NSignal_Unit::U_insulin, 0xFFF49083, 0xFFF49083, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor del_ibr_desc{ scgms::signal_Delivered_Insulin_Basal_Rate, dsSignal_Delivered_Insulin_Basal_Rate, dsU_per_Hr, scgms::NSignal_Unit::U_per_Hr, 0xFFF59053, 0xFFF59053, scgms::NSignal_Visualization::step, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor del_iidr_desc{ scgms::signal_Delivered_Insulin_Intradermal_Rate, dsSignal_Delivered_Insulin_Intradermal_Rate, dsU_per_Hr, scgms::NSignal_Unit::U_per_Hr, 0xFFF54053, 0xFFF54053, scgms::NSignal_Visualization::step, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor del_ihins_desc{ scgms::signal_Delivered_Insulin_Inhaled, dsSignal_Delivered_Insulin_Inhaled, dsU, scgms::NSignal_Unit::U_insulin, 0xFF859053, 0xFF859053, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor ins_act_desc{ scgms::signal_Insulin_Activity, dsSignal_GUI_Name_Insulin_Activity, L"", scgms::NSignal_Unit::Other, 0xFF0055DD, 0xFF0055DD, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor iob_desc{ scgms::signal_IOB, dsSignal_GUI_Name_IOB, L"", scgms::NSignal_Unit::Other, 0xFFDD5555, 0xFFDD5555, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor cob_desc{ scgms::signal_COB, dsSignal_GUI_Name_COB, L"", scgms::NSignal_Unit::Other, 0xFF55DD55, 0xFF55DD55, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor cho_in_desc{ scgms::signal_Carb_Intake, dsSignal_GUI_Name_Carbs, L"", scgms::NSignal_Unit::g, 0xF00FF00, 0xF00FF00, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor cho_resc_desc{ scgms::signal_Carb_Rescue, dsSignal_GUI_Name_Carb_Rescue, L"", scgms::NSignal_Unit::g, 0xFF80FF80, 0xFF80FF80, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor phys_act_desc{ scgms::signal_Physical_Activity, dsSignal_GUI_Name_Physical_Activity, L"", scgms::NSignal_Unit::Percent, 0xFF80FFFF, 0xFF80FFFF, scgms::NSignal_Visualization::step, scgms::NSignal_Mark::none, nullptr };

	const scgms::TSignal_Descriptor skin_temp_desc{ scgms::signal_Skin_Temperature, dsSignal_GUI_Name_Skin_Temperature, L"", scgms::NSignal_Unit::Percent, 0xFF808080, 0xFF808080, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor air_temp_desc{ scgms::signal_Air_Temperature, dsSignal_GUI_Name_Air_Temperature, L"", scgms::NSignal_Unit::Percent, 0xFF808080, 0xFF808080, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor heartbeat_desc{ scgms::signal_Heartbeat, dsSignal_GUI_Name_Heartbeat, L"", scgms::NSignal_Unit::Percent, 0xFF808080, 0xFF808080, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor eda_desc{ scgms::signal_Electrodermal_Activity , dsSignal_GUI_Name_Electrodermal_Activity, L"", scgms::NSignal_Unit::Percent, 0xFF808080, 0xFF808080, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor steps_desc{ scgms::signal_Steps, dsSignal_GUI_Name_Steps, L"", scgms::NSignal_Unit::Percent, 0xFF808080, 0xFF808080, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor sleep_quality_desc{ scgms::signal_Sleep_Quality, dsSignal_GUI_Name_Sleep_Quality, L"", scgms::NSignal_Unit::Percent, 0xFF808080, 0xFF808080, scgms::NSignal_Visualization::step, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor accel_desc{ scgms::signal_Acceleration, dsSignal_GUI_Name_Acceleration, L"", scgms::NSignal_Unit::Percent, 0xFF808080, 0xFF808080, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor movspeed_desc{ scgms::signal_Movement_Speed, dsSignal_GUI_Name_Movement_Speed, L"", scgms::NSignal_Unit::m_per_s, 0xFF0080FF, 0xFF0080FF, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

	const scgms::TSignal_Descriptor ins_sens_desc{ scgms::signal_Insulin_Sensitivity, dsSignal_GUI_Name_Insulin_Sensitivity, L"", scgms::NSignal_Unit::Other, 0xFF0088DD, 0xFF0088DD, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor carb_ratio_desc{ scgms::signal_Carb_Ratio, dsSignal_GUI_Name_Carb_Ratio, L"", scgms::NSignal_Unit::Other, 0xFF00DD88, 0xFF00DD88, scgms::NSignal_Visualization::smooth, scgms::NSignal_Mark::none, nullptr };

	const std::array<scgms::TSignal_Descriptor, 28> signals = { {bg_desc, bg_cal_desc, ig_desc, isig_desc, req_bolus_desc, req_ibr_desc, del_bolus_desc, del_ins_total_desc, del_ibr_desc, ins_act_desc, iob_desc, cob_desc, cho_in_desc, cho_resc_desc, phys_act_desc,
																 skin_temp_desc, air_temp_desc, heartbeat_desc, eda_desc, steps_desc, accel_desc, sleep_quality_desc, ins_sens_desc, carb_ratio_desc, req_iidr_desc, del_iidr_desc, del_ihins_desc, movspeed_desc} };
}

namespace feedback_sender {
	constexpr size_t param_count = 3;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptWChar_Array,
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSignal_Source_Id,
		dsRemove_From_Source,
		dsFeedback_Name,
	};

	const wchar_t* config_param_name[param_count] = {
		rsSignal_Source_Id,
		rsRemove_From_Source,
		rsFeedback_Name,
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		nullptr,
		nullptr,
	};

	const scgms::TFilter_Descriptor desc = {
			{ 0x5d29ea43, 0x4fac, 0x4141, { 0xa0, 0x3f, 0x73, 0x3b, 0x10, 0x29, 0x67, 0x27 } }, //// {5D29EA43-4FAC-4141-A03F-733B10296727},
		scgms::NFilter_Flags::None,
		dsSignal_Feedback,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

namespace impulse_response {

	constexpr const GUID id = { 0x24ee7711, 0xb2b2, 0x45f4, { 0x94, 0xf, 0xad, 0x77, 0x53, 0x96, 0xb9, 0xb5 } }; // {24EE7711-B2B2-45F4-940F-AD775396B9B5}

	constexpr size_t param_count = 2;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptRatTime,
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSignal_Id,
		dsResponse_Window,
	};

	const wchar_t* config_param_name[param_count] = {
		rsSignal_Id,
		rsResponse_Window,
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		nullptr,
	};

	const scgms::TFilter_Descriptor desc = {
		id,
		scgms::NFilter_Flags::None,
		dsImpulse_Response_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

namespace noise_generator {

	constexpr const GUID id = { 0xe9924a75, 0xcfd3, 0x4d79, { 0xa8, 0x7b, 0x65, 0x4b, 0x8f, 0x10, 0x97, 0xa5 } };	// {E9924A75-CFD3-4D79-A87B-654B8F1097A5}

	constexpr size_t param_count = 2;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptSignal_Id,
		scgms::NParameter_Type::ptDouble,
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSignal_Id,
		L"Noise maximum value",
	};

	const wchar_t* config_param_name[param_count] = {
		rsSignal_Id,
		L"Noise_Max",
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		nullptr,
	};

	const scgms::TFilter_Descriptor desc = {
		id,
		scgms::NFilter_Flags::None,
		L"White noise generator filter",
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

const std::array<scgms::TFilter_Descriptor, 10> filter_descriptions = { { calculate::Calculate_Descriptor, mapping::Mapping_Descriptor, decoupling::desc, masking::Masking_Descriptor, unmasking::Unmasking_Descriptor, signal_generator::desc, network_signal_generator::desc, feedback_sender::desc, impulse_response::desc, noise_generator::desc } };


extern "C" HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {
	*begin = const_cast<scgms::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor * *begin, scgms::TSignal_Descriptor * *end) {
	*begin = const_cast<scgms::TSignal_Descriptor*>(signal_descriptor::signals.data());
	*end = *begin + signal_descriptor::signals.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter) {	
	if (*id == calculate::Calculate_Descriptor.id)
		return Manufacture_Object<CCalculate_Filter>(filter, output);
	else if (*id == masking::Masking_Descriptor.id)
		return Manufacture_Object<CMasking_Filter>(filter, output);
	else if (*id == unmasking::Unmasking_Descriptor.id)
		return Manufacture_Object<CUnmasking_Filter>(filter, output);
	else if (*id == mapping::Mapping_Descriptor.id)
		return Manufacture_Object<CMapping_Filter>(filter, output);
	else if (*id == decoupling::desc.id)
		return Manufacture_Object<CDecoupling_Filter>(filter, output);
	// NOTE: signal generator and network variant shares one implementation, the latter having an extra set of parameters
	else if (*id == signal_generator::desc.id || *id == network_signal_generator::desc.id)
		return Manufacture_Object<CSignal_Generator>(filter, output);
	else if (*id == feedback_sender::desc.id)
		return Manufacture_Object<CSignal_Feedback>(filter, output);
	else if (*id == impulse_response::desc.id)
		return Manufacture_Object<CImpulse_Response_Filter>(filter, output);
	else if (*id == noise_generator::desc.id)
		return Manufacture_Object<CWhite_Noise_Generator_Filter>(filter, output);

	return E_NOTIMPL;
}

extern "C" HRESULT IfaceCalling do_create_signal(const GUID *signal_id, scgms::ITime_Segment *segment, const GUID * approx_id, scgms::ISignal **signal) {
	if (signal_id == nullptr)	//signal error sets segment to nullptr as it does not need it, so we check signal_id only
		return E_INVALIDARG;

	for (const auto& supported_signal : signal_descriptor::signals) {
		if (supported_signal.id == *signal_id)
			return Manufacture_Object<CMeasured_Signal>(signal, approx_id);
	}

	return E_FAIL;
}
