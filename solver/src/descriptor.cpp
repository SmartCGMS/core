#include "descriptor.h"
#include "solver.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace compute
{
	constexpr size_t param_count = 17;

	// TODO: add model bounds to parameters; this could be done quite easily here in backend by adding "model parameters vector" NParameter_Type and working with it just like that
	//		 but in user interface, there needs to be dynamic widget which changes contents according to selected model; for now, use default model parameter bounds

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptModel_Id,
		glucose::NParameter_Type::ptMetric_Id,
		glucose::NParameter_Type::ptSolver_Id,
		glucose::NParameter_Type::ptModel_Signal_Id,
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
		dsSelected_Metric,
		dsSelected_Solver,
		dsSelected_Signal,
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
		rsSelected_Metric,
		rsSelected_Solver,
		rsSelected_Signal,
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
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		dsHold_During_Solve_Tooltip
	};

	const glucose::TFilter_Descriptor Compute_Descriptor = {
		{ 0xc0e942b9, 0x3928, 0x4b81, { 0x9b, 0x43, 0xa3, 0x47, 0x66, 0x82, 0x0, 0x99 } },	//// {C0E942B9-3928-4B81-9B43-A34766820099}
		dsSolver_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

const std::array<glucose::TFilter_Descriptor, 1> filter_descriptions = { compute::Compute_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == compute::Compute_Descriptor.id)
		return Manufacture_Object<CCompute_Filter>(filter, refcnt::make_shared_reference_ext<glucose::SFilter_Pipe, glucose::IFilter_Pipe>(input, true), refcnt::make_shared_reference_ext<glucose::SFilter_Pipe, glucose::IFilter_Pipe>(output, true));

	return ENOENT;
}
