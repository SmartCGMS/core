#include "descriptor.h"
#include "calculate.h"
#include "mapping.h"
#include "Measured_Signal.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace calculate
{
	constexpr size_t param_count = 4;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptModel_Id,
		glucose::NParameter_Type::ptModel_Signal_Id,
		glucose::NParameter_Type::ptBool,
		glucose::NParameter_Type::ptBool
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSelected_Model,
		dsSelected_Signal,
		dsCalculate_Past_New_Params
	};

	const wchar_t* config_param_name[param_count] = {
		rsSelected_Model,
		rsSelected_Signal,
		rsRecalculate_Past_On_Params,
		rsRecalculate_Past_On_Segment_Stop
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};

	const glucose::TFilter_Descriptor Calculate_Descriptor = {
		{ 0x14a25f4c, 0xe1b1, 0x85c4, { 0x12, 0x74, 0x9a, 0x0d, 0x11, 0xe0, 0x98, 0x13 } }, //// {14A25F4C-E1B1-85C4-1274-9A0D11E09813}
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
		nullptr,
		nullptr
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

namespace measured_signal
{
	constexpr size_t supported_count = 5;

	const GUID supported_signal_ids[supported_count] = {
		glucose::signal_IG,
		glucose::signal_BG,
		glucose::signal_Insulin,
		glucose::signal_Carb_Intake,
		glucose::signal_Calibration
	};
}

const std::array<glucose::TFilter_Descriptor, 2> filter_descriptions = { calculate::Calculate_Descriptor, mapping::Mapping_Descriptor };

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

	return ENOENT;
}

HRESULT IfaceCalling do_create_measured_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) {	

	if ((calc_id == nullptr) || (segment == nullptr))
		return E_INVALIDARG;

	for (size_t i = 0; i < measured_signal::supported_count; i++)
	{
		if (measured_signal::supported_signal_ids[i] == *calc_id)
			return Manufacture_Object<CMeasured_Signal>(signal);
	}

	return E_FAIL;
}
