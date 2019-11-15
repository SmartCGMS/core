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

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/descriptor_utils.h"
#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/rtl/FilterLib.h"

#include "descriptor.h"

#include "absoprtion/iob.h"
#include "absoprtion/cob.h"
#include "betapid/betapid.h"

#include <vector>

namespace iob {
	const GUID model_id = { 0xd3d57cb4, 0x48da, 0x40e2, { 0x9e, 0x53, 0xbb, 0x1e, 0x84, 0x8a, 0x63, 0x95 } };// {D3D57CB4-48DA-40E2-9E53-BB1E848A6395}


	constexpr GUID signal_Insulin_Activity_Bilinear = { 0x235962db, 0x1356, 0x4d14, { 0xb8, 0x54, 0x49, 0x3f, 0xd9, 0x79, 0x1, 0x42 } };		// {235962DB-1356-4D14-B854-493FD9790142}
	constexpr GUID signal_Insulin_Activity_Exponential = { 0x19974ff5, 0x9eb1, 0x4a99, { 0xb7, 0xf2, 0xdc, 0x9b, 0xfa, 0xe, 0x31, 0x5e } };	// {19974FF5-9EB1-4A99-B7F2-DC9BFA0E315E}
	constexpr GUID signal_IOB_Bilinear = { 0xa8fa0190, 0x4c89, 0x4e0b, { 0x88, 0x55, 0xaf, 0x47, 0xba, 0x29, 0x4c, 0xff } };		// {A8FA0190-4C89-4E0B-8855-AF47BA294CFF}
	constexpr GUID signal_IOB_Exponential = { 0x238d2353, 0x6d37, 0x402c, { 0xaf, 0x39, 0x6c, 0x55, 0x52, 0xa7, 0x7e, 0x1f } };	// {238D2353-6D37-402C-AF39-6C5552A77E1F}


	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptTime, glucose::NModel_Parameter_Value::mptTime };

	const wchar_t *param_names[param_count] = { dsInsulinPeak, dsDIA };
	const wchar_t *param_columns[param_count] = { rsInsulinPeak, rsDIA };

	const double lower_bound[param_count] = { 30.0*glucose::One_Minute };
	const double upper_bound[param_count] = { 24.0*glucose::One_Hour };

	const size_t signal_count = 4;

	const GUID signal_ids[signal_count] = { signal_Insulin_Activity_Bilinear, signal_Insulin_Activity_Exponential, signal_IOB_Bilinear, signal_IOB_Exponential };
	const wchar_t *signal_names[signal_count] = { dsInsulin_Activity_Bilinear, dsInsulin_Activity_Exponential, dsIOB_Bilinear, dsIOB_Exponential };
	const GUID reference_signal_ids[signal_count] = { glucose::signal_IG, glucose::signal_IG, glucose::signal_IG, glucose::signal_IG }; /* we use BG as "trigger" for recalculation */

	const glucose::TModel_Descriptor desc = {
		model_id,
		dsIOB_Model,
		rsIOB_Model,
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
		reference_signal_ids,
		false,
		nullptr
	};
}

namespace cob {
	const GUID model_id = { 0xe63c23e4, 0x7932, 0x4c47, { 0x9c, 0xea, 0xa7, 0xa6, 0x7f, 0x75, 0x17, 0x23 } };// {E63C23E4-7932-4C47-9CEA-A7A67F751723}

	constexpr GUID signal_COB_Bilinear = { 0xe29a9d38, 0x551e, 0x4f3f, { 0xa9, 0x1d, 0x1f, 0x14, 0xd9, 0x34, 0x67, 0xe3 } };		// {E29A9D38-551E-4F3F-A91D-1F14D93467E3}

	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptTime, glucose::NModel_Parameter_Value::mptTime };

	const wchar_t *param_names[param_count] = { dsCarbPeak, dsCarbDA };
	const wchar_t *param_columns[param_count] = { rsCarbPeak, rsCarbDA };

	const double lower_bound[param_count] = { 30.0*glucose::One_Minute };
	const double upper_bound[param_count] = { 24.0*glucose::One_Hour };

	const size_t signal_count = 1;

	const GUID signal_ids[signal_count] = { signal_COB_Bilinear };
	const wchar_t *signal_names[signal_count] = { dsCOB_Bilinear };
	const GUID reference_signal_ids[signal_count] = { glucose::signal_IG }; /* we use BG as "trigger" for recalculation */

	const glucose::TModel_Descriptor desc = {
		model_id,
		dsCOB_Model,
		rsCOB_Model,
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
		reference_signal_ids,
		false,
		nullptr,
		nullptr
	};
}

namespace betapid_insulin_regulation {
	const GUID id = { 0x1dc11fd, 0xb12f, 0x4eb1, { 0x97, 0xa3, 0xa, 0xea, 0x93, 0xa2, 0xdf, 0xa6 } };	// {01DC11FD-B12F-4EB1-97A3-0AEA93A2DFA6}

	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble };

	const wchar_t *param_names[param_count] = { dsKp, dsKi, dsKd, dsISF, dsCSR, dsBIN };
	const wchar_t *param_columns[param_count] = { rsKp, rsKi, rsKd, rsISF, rsCSR, rsBIN };

	const double lower_bound[param_count] = { -10.0, -10.0, -10.0, 0, 0, 0 };
	const double upper_bound[param_count] = { 10.0, 10.0, 10.0, 2.0, 2.0, 2.0 };

	const size_t signal_count = 2;

	const GUID signal_ids[signal_count] = { betapid_signal_id, betapid2_signal_id };
	const wchar_t *signal_names[signal_count] = { dsInsulin_BetaPID_Rate, dsInsulin_BetaPID2_Rate };
	const GUID reference_signal_ids[signal_count] = { glucose::signal_IOB, glucose::signal_COB };

	const glucose::TModel_Descriptor desc = {
		id,
		dsBetaPID,
		rsBetaPID,
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
		reference_signal_ids,
		false,
		nullptr,
		nullptr
	};
}

namespace betapid3_insulin_regulation {
	const GUID id = { 0x83b5d264, 0x401e, 0x4bf1, { 0x97, 0x44, 0x27, 0x6e, 0xc0, 0x61, 0x5b, 0x72 } }; // {83B5D264-401E-4BF1-9744-276EC0615B72}

	const glucose::NModel_Parameter_Value param_types[param_count] = { glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble,
		glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble, glucose::NModel_Parameter_Value::mptDouble };

	const wchar_t *param_names[param_count] = { dsParam_K, dsKi, dsKd, dsKiDecay, dsBIN };
	const wchar_t *param_columns[param_count] = { rsParam_K, rsKi, rsKd, rsKiDecay, rsBIN };

	const double lower_bound[param_count] = { -10.0, -10.0, -10.0, 0, 0 };
	const double upper_bound[param_count] = { 10.0, 10.0, 10.0, 1.0, 2.0 };

	const size_t signal_count = 1;

	const GUID signal_ids[signal_count] = { betapid3_signal_id };
	const wchar_t *signal_names[signal_count] = { dsInsulin_BetaPID3_Rate };
	const GUID reference_signal_ids[signal_count] = { glucose::signal_Carb_Ratio };

	const GUID effect_reference_signal_ids[signal_count] = { glucose::signal_Constant_BG };
	const GUID effect_signal_ids[signal_count] = { glucose::signal_IG };

	const glucose::TModel_Descriptor desc = {
		id,
		dsBetaPID3,
		rsBetaPID3,
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
		reference_signal_ids,
		true,
		effect_reference_signal_ids,
		effect_signal_ids,
	};
}

const std::array<glucose::TModel_Descriptor, 2> model_descriptions = { { iob::desc, cob::desc, betapid_insulin_regulation::desc, betapid3_insulin_regulation::desc } };

extern "C" HRESULT IfaceCalling do_get_model_descriptors(glucose::TModel_Descriptor **begin, glucose::TModel_Descriptor **end) {
	*begin = const_cast<glucose::TModel_Descriptor*>(model_descriptions.data());
	*end = *begin + model_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) {
	if ((calc_id == nullptr) || (segment == nullptr)) return E_INVALIDARG;

	glucose::WTime_Segment weak_segment{ segment };

	if (*calc_id == glucose::signal_Insulin_Activity_Bilinear)
		return Manufacture_Object<CInsulin_Absorption_Bilinear, glucose::ISignal>(signal, weak_segment, NInsulin_Calc_Mode::Activity);
	else if (*calc_id == glucose::signal_Insulin_Activity_Exponential)
		return Manufacture_Object<CInsulin_Absorption_Exponential, glucose::ISignal>(signal, weak_segment, NInsulin_Calc_Mode::Activity);
	else if (*calc_id == glucose::signal_IOB_Bilinear)
		return Manufacture_Object<CInsulin_Absorption_Bilinear, glucose::ISignal>(signal, weak_segment, NInsulin_Calc_Mode::IOB);
	else if (*calc_id == glucose::signal_IOB_Exponential)
		return Manufacture_Object<CInsulin_Absorption_Exponential, glucose::ISignal>(signal, weak_segment, NInsulin_Calc_Mode::IOB);
	else if (*calc_id == glucose::signal_COB_Bilinear)
		return Manufacture_Object<CCarbohydrates_On_Board_Bilinear, glucose::ISignal>(signal, weak_segment);
	else if (*calc_id == betapid_insulin_regulation::betapid_signal_id)
		return Manufacture_Object<CBetaPID_Insulin_Regulation, glucose::ISignal>(signal, weak_segment);
	else if (*calc_id == betapid_insulin_regulation::betapid2_signal_id)
		return Manufacture_Object<CBetaPID2_Insulin_Regulation, glucose::ISignal>(signal, weak_segment);
	else if (*calc_id == betapid3_insulin_regulation::betapid3_signal_id)
		return Manufacture_Object<CBetaPID3_Insulin_Regulation, glucose::ISignal>(signal, weak_segment);

	return E_NOTIMPL;
}
