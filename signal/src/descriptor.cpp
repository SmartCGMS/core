#include "descriptor.h"
#include "calculate.h"
#include "mapping.h"
#include "masking.h"
#include "Measured_Signal.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace calculate
{
	constexpr size_t param_count = 3;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptModel_Id,
		glucose::NParameter_Type::ptModel_Signal_Id,
		glucose::NParameter_Type::ptRatTime
	};

	const wchar_t* ui_param_name[param_count] = {
		dsSelected_Model,
		dsSelected_Signal,
		dsPrediction_Window
	};

	const wchar_t* config_param_name[param_count] = {
		rsSelected_Model,
		rsSelected_Signal,
		rsPrediction_Window
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		nullptr,
		nullptr
	};

	const glucose::TFilter_Descriptor Calculate_Descriptor = {
		{ 0x14a25f4c, 0xe1b1, 0x85c4,{ 0x12, 0x74, 0x9a, 0x0d, 0x11, 0xe0, 0x98, 0x13 }},  // {14A25F4C-E1B1-85C4-1274-9A0D11E09813}
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
		nullptr,
		nullptr
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
	constexpr size_t supported_count = 5;

	const GUID supported_signal_ids[supported_count] = {
		glucose::signal_IG,
		glucose::signal_BG,
		glucose::signal_Insulin,
		glucose::signal_Carb_Intake,
		glucose::signal_Calibration
	};
}

const std::array<glucose::TFilter_Descriptor, 3> filter_descriptions = { calculate::Calculate_Descriptor, mapping::Mapping_Descriptor, masking::Masking_Descriptor };

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
