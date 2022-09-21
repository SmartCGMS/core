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

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include "p559/p559.h"
#include "aim_ge/aim_ge.h"
#include "enhacement/data_enhacement.h"
#include "cgp_pred/cgp_pred.h"
#include "bases/bases.h"

#include <vector>

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
		L"b10_a", L"b10_toff", L"b10_var",
		L"b11_a", L"b11_toff", L"b11_var",
		/*L"b12_a", L"b12_toff", L"b12_var",
		L"b13_a", L"b13_toff", L"b13_var",
		L"b14_a", L"b14_toff", L"b14_var",
		L"b15_a", L"b15_toff", L"b15_var",*/

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
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		/*scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,
		scgms::NModel_Parameter_Value::mptDouble,scgms::NModel_Parameter_Value::mptTime,scgms::NModel_Parameter_Value::mptDouble,*/

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

const std::array<scgms::TModel_Descriptor, 5> model_descriptions = { {  p559_model::desc,
																		 aim_ge::desc,
																		 data_enhacement::desc,
																		 cgp_pred::desc,
																		 bases_pred::desc,
																		} };

const std::array<scgms::TSignal_Descriptor, 5+cgp_pred::number_of_calculated_signals> signals_descriptors = { {
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
	if (*model_id == p559_model::model_id) return Manufacture_Object<CP559_Model>(model, parameters, output);
	else if (*model_id == aim_ge::model_id) return Manufacture_Object<CAIM_GE_Model>(model, parameters, output);
	else if (*model_id == data_enhacement::model_id) return Manufacture_Object<CData_Enhacement_Model>(model, parameters, output);
	else if (*model_id == cgp_pred::model_id) return Manufacture_Object<CCGP_Prediction>(model, parameters, output);
	else if (*model_id == bases_pred::model_id) return Manufacture_Object<CBase_Functions_Predictor>(model, parameters, output);
		else return E_NOTIMPL;
}

HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {
	// placeholder, will be utilized in future
	*begin = nullptr;
	*end = *begin;
	return S_OK;
}

HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *output, scgms::IFilter **filter) {
	// placeholder, will be utilized in future
	return E_NOTIMPL;
}