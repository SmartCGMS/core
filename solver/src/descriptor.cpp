/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#include "descriptor.h"
#include "solver.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include "../../../common/rtl/descriptor_utils.h"

#include <vector>

namespace compute
{
	constexpr size_t param_count = 17;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptModel_Id,
		glucose::NParameter_Type::ptModel_Signal_Id,
		glucose::NParameter_Type::ptMetric_Id,
		glucose::NParameter_Type::ptSolver_Id,
		glucose::NParameter_Type::ptModel_Bounds,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptDouble,
		glucose::NParameter_Type::ptInt64,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptInt64,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSelected_Model,
		dsSelected_Signal,
		dsSelected_Metric,
		dsSelected_Solver,
		dsSelected_Model_Bounds,
		dsUse_Relative_Error,
		dsUse_Squared_Diff,
		dsUse_Prefer_More_Levels,
		dsMetric_Threshold,
		dsMetric_Levels_Required,
		dsUse_Measured_Levels,
		dsRecalculate_On_Levels_Count,
		dsRecalculate_On_Segment_End,
		dsRecalculate_On_Calibration,
		dsRecalculate_With_Every_Params,
		dsUse_Just_Opened_Segments,
		dsHold_During_Solve
	};

	const wchar_t* config_param_name[param_count] = {
		rsSelected_Model,
		rsSelected_Signal,
		rsSelected_Metric,
		rsSelected_Solver,
		rsSelected_Model_Bounds,
		rsUse_Relative_Error,
		rsUse_Squared_Diff,
		rsUse_Prefer_More_Levels,
		rsMetric_Threshold,
		rsMetric_Levels_Required,
		rsUse_Measured_Levels,
		rsRecalculate_On_Levels_Count,
		rsRecalculate_On_Segment_End,
		rsRecalculate_On_Calibration,
		rsRecalculate_With_Every_Params,
		rsUse_Just_Opened_Segments,
		rsHold_During_Solve
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsSelected_Model_Tooltip,
		dsSelected_Signal_Tooltip,
		dsSelected_Metric_Tooltip,
		dsSelected_Solver_Tooltip,
		nullptr,
		dsUse_Relative_Error_Tooltip,
		dsUse_Squared_Diff_Tooltip,
		dsUse_Prefer_More_Levels_Tooltip,
		dsMetric_Threshold_Tooltip,
		dsMetric_Levels_Required_Hint,
		dsUse_Measured_Levels_Tooltip,
		dsRecalculate_On_Levels_Count_Tooltip,
		dsRecalculate_On_Segment_End_Tooltip,
		dsRecalculate_On_Calibration_Tooltip,
		dsRecalculate_On_Parameters_Tooltip,
		dsUse_Opened_Segments_Only_Tooltip,
		dsHold_During_Solve_Tooltip
	};

	const glucose::TFilter_Descriptor Compute_Descriptor = {
		solver_id,
		dsSolver_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

const std::array<glucose::TFilter_Descriptor, 1> filter_descriptions = { { compute::Compute_Descriptor } };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	return do_get_descriptors(filter_descriptions, begin, end);
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == compute::Compute_Descriptor.id)
	{
		glucose::SFilter_Pipe shared_in = refcnt::make_shared_reference_ext<glucose::SFilter_Pipe, glucose::IFilter_Pipe>(input, true);
		glucose::SFilter_Pipe shared_out = refcnt::make_shared_reference_ext<glucose::SFilter_Pipe, glucose::IFilter_Pipe>(output, true);

		return Manufacture_Object<CCompute_Filter>(filter, shared_in, shared_out);
	}

	return ENOENT;
}
