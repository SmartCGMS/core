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

#include "masking.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/utils/string_utils.h"

#include <algorithm>
#include <cctype>

CMasking_Filter::CMasking_Filter(scgms::IFilter *output) : CBase_Filter(output) {
	//
}

bool CMasking_Filter::Parse_Bitmask(std::wstring inw)
{
	if (inw.empty())
		return false;
	
	std::string in = Narrow_WString(inw);

	// erase spaces - typically the string is split to groups
	in.erase(std::remove_if(in.begin(), in.end(), [](unsigned char x) { return std::isspace(static_cast<int>(x)); }), in.end());

	mBitCount = in.length();

	// limit bit count from both sides
	if (mBitCount < BitmaskMinBitCount || mBitCount > BitmaskMaxBitCount)
		return false;

	// parse bitmask - only 0's and 1's are allowed
	for (size_t i = 0; i < mBitCount; i++)
	{
		switch (in[i])
		{
			//zeroes represent levels to suppres
			//ones represent levels to pass through
			case L'1': mMask.reset(i); break;
			case L'0': mMask.set(i);   break;
			default:   return false;
		}
	}

	return true;
}

HRESULT IfaceCalling CMasking_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mSignal_Id = configuration.Read_GUID(rsSignal_Masked_Id);
	if (Is_Invalid_GUID(mSignal_Id)) return E_INVALIDARG;

	const std::wstring mask = configuration.Read_String(rsSignal_Value_Bitmask);
	if (!Parse_Bitmask(mask)) {
		std::wstring desc = dsFilter_configuration_param_value_error;
		desc += dsMasking_Filter;
		desc += L" (2) ";
		desc += dsSignal_Value_Bitmask;
		desc += L" (3) ";
		desc += mask;
		error_description.push(desc);

		return E_FAIL;
	}

	return S_OK;
}

HRESULT IfaceCalling CMasking_Filter::Do_Execute(scgms::UDevice_Event event) {
	// mask only configured signal and event of type "Level"
	if (event.event_code() == scgms::NDevice_Event_Code::Level && event.signal_id() == mSignal_Id) {
		auto itr = mSegmentMaskState.find(event.segment_id());
		if (itr == mSegmentMaskState.end())
			mSegmentMaskState[event.segment_id()] = 0;

		if (mMask.test(mSegmentMaskState[event.segment_id()]))
		{
			// This is certainly not nice, but we want to avoid creating a new device event, because it will generate new logical clock value, which would be
			// out of order with the rest of levels; also Level and Masked_Level uses the same union value, so it's fine to just change the type
			scgms::TDevice_Event* raw;
			if (event.get()->Raw(&raw) == S_OK)
				raw->event_code = scgms::NDevice_Event_Code::Masked_Level;
		}

		mSegmentMaskState[event.segment_id()] = (mSegmentMaskState[event.segment_id()] + static_cast<uint64_t>(1)) % mBitCount;
	}


	return mOutput.Send(event);
}