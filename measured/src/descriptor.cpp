#pragma once

#include "descriptor.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"
#include "Measured_Signal.h"
#include <vector>

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

HRESULT IfaceCalling do_create_measured_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal)
{
	if ((calc_id == nullptr) || (segment == nullptr))
		return E_INVALIDARG;

	for (size_t i = 0; i < measured_signal::supported_count; i++)
	{
		if (measured_signal::supported_signal_ids[i] == *calc_id)
			return Manufacture_Object<CMeasured_Signal>(signal);
	}

	return E_FAIL;
}
