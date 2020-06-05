/* Examples and Documentation for
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
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "descriptor.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/SolverLib.h"
#include "../../../common/utils/descriptor_utils.h"
#include "../../../common/rtl/manufactory.h"

#include "oref.h"

#include <vector>

namespace oref_model {

	const scgms::NModel_Parameter_Value param_types[param_count] = { scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble, scgms::NModel_Parameter_Value::mptDouble };

	const wchar_t *param_names[param_count] = { dsISF, L"CR", dsBasal_Insulin_Rate };
	const wchar_t *param_columns[param_count] = { rsISF, L"CR", rsBasal_Insulin_Rate };

	const size_t signal_count = 2;

	const GUID signal_ids[signal_count] = { oref0_basal_rate_signal_id, oref0_smb_signal_id };
	const wchar_t *signal_names[signal_count] = { L"oref0 basal insulin rate", L"oref0 super-microbolus" };
	const GUID reference_signal_ids[signal_count] = { scgms::signal_IOB };

	const scgms::TModel_Descriptor desc = {
		model_id,
		scgms::NModel_Flags::Discrete_Model,
		L"oref basal insulin",
		L"oref_basal_insulin",
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

	const scgms::TSignal_Descriptor oref0_basal_desc{ oref0_basal_rate_signal_id, signal_names[0], dsU_per_Hr, scgms::NSignal_Unit::U_per_Hr, 0xFF008000, 0xFF008000, scgms::NSignal_Visualization::step, scgms::NSignal_Mark::none, nullptr };
	const scgms::TSignal_Descriptor oref0_smb_desc{ oref0_smb_signal_id, signal_names[1], dsU, scgms::NSignal_Unit::U_insulin, 0xFF008000, 0xFF008000, scgms::NSignal_Visualization::mark, scgms::NSignal_Mark::none, nullptr };
}

const std::array<scgms::TModel_Descriptor, 1> model_descriptors = { { oref_model::desc } };

const std::array<scgms::TSignal_Descriptor, 2> signal_descriptors = { { oref_model::oref0_basal_desc, oref_model::oref0_smb_desc } };


extern "C" HRESULT IfaceCalling do_get_model_descriptors(scgms::TModel_Descriptor **begin, scgms::TModel_Descriptor **end) {

	return do_get_descriptors(model_descriptors, begin, end);
}

extern "C" HRESULT IfaceCalling do_get_signal_descriptors(scgms::TSignal_Descriptor **begin, scgms::TSignal_Descriptor **end) {

	return do_get_descriptors(signal_descriptors, begin, end);
}

extern "C" HRESULT IfaceCalling do_create_discrete_model(const GUID *model_id, scgms::IModel_Parameter_Vector *parameters, scgms::IFilter *output, scgms::IDiscrete_Model **model) {

	if (*model_id == oref_model::model_id) {
		return Manufacture_Object<COref_Discrete_Model>(model, parameters, output);
	}

	return E_NOTIMPL;
}
