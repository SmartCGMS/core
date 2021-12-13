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

#include "median_response_filter.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"

#include <algorithm>
#include <numeric>

CMedian_Response_Filter::CMedian_Response_Filter(scgms::IFilter *output) : CBase_Filter(output) {
	//
}


HRESULT IfaceCalling CMedian_Response_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mSignal_Id = configuration.Read_GUID(rsSignal_Id);
	mTime_Window = configuration.Read_Double(rsResponse_Window);

	if (mTime_Window < 0)
		return E_INVALIDARG;

	return S_OK;
}

HRESULT IfaceCalling CMedian_Response_Filter::Do_Execute(scgms::UDevice_Event event)
{
	if (event.event_code() == scgms::NDevice_Event_Code::Time_Segment_Start)
		mLast_Time.emplace(event.segment_id(), std::numeric_limits<double>::lowest());
	else if (event.event_code() == scgms::NDevice_Event_Code::Time_Segment_Stop)
		mLast_Time.erase(event.segment_id());

	if (event.signal_id() == mSignal_Id && event.is_level_event())
	{
		if (event.device_time() < mLast_Time[event.segment_id()])
			return E_FAIL;

		event.level() = Median_Response(event.segment_id(), event.device_time(), event.level());
	}

	return mOutput.Send(event);
}

double CMedian_Response_Filter::Median_Response(const uint64_t time_segment, const double time, const double value)
{
	auto& values = mValues[time_segment];
	const auto lastTime = std::max(mLast_Time[time_segment], time);

	if (mTime_Window > std::numeric_limits<double>::epsilon())
	{
		const double minTime = lastTime - mTime_Window;

		while (!values.empty() && (*values.begin()).first < minTime)
			values.pop_front();
	}

	values.emplace_back(time, value);

	mLast_Time[time_segment] = lastTime;

	std::vector<double> signal_values;
	for (auto& val : values)
		signal_values.push_back(val.second);

	std::sort(signal_values.begin(), signal_values.end());

	auto first = signal_values.begin();
	auto last = signal_values.end();
	double median = std::numeric_limits<double>::quiet_NaN();
	if ((signal_values.size() % 2) == 1)
	{
		auto middle = first + (last - first) / 2;
		median = *middle;
	}
	else if (signal_values.size() != 0)
	{
		auto middle_upper = first + (last - first) / 2;
		auto middle_lower = first + (last - first) / 2 - 1;
		median = (*middle_upper + *middle_lower) / 2.0;
	}

	return median;
}
