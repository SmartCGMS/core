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
 *       monitoring", Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 */

#include "masking.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"

#include <algorithm>
#include <cctype>
#include <codecvt>
#include <locale>

CMasking_Filter::CMasking_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{ inpipe }, mOutput{ outpipe }
{
	//
}

bool CMasking_Filter::Parse_Bitmask(std::wstring inw)
{
	if (inw.empty())
		return false;

	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
	std::string in = converterX.to_bytes(inw);

	// erase spaces - typically the string is split to groups
	in.erase(std::remove_if(in.begin(), in.end(), [](unsigned char x) {return std::isspace((int)x); }), in.end());

	mBitCount = in.length();

	// limit bit count from both sides
	if (mBitCount < BitmaskMinBitCount || mBitCount > BitmaskMaxBitCount)
		return false;

	// parse bitmask - only 0's and 1's are allowed
	for (size_t i = 0; i < mBitCount; i++)
	{
		switch (in[i])
		{
			case L'0': mMask.reset(i); break;
			case L'1': mMask.set(i);   break;
			default:   return false;
		}
	}

	return true;
}

HRESULT CMasking_Filter::Run(glucose::IFilter_Configuration* configuration) {
	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);
	mSignal_Id = shared_configuration.Read_GUID(rsSignal_Masked_Id);

	if (!Parse_Bitmask(shared_configuration.Read_String(rsSignal_Value_Bitmask)))
		return E_FAIL;

	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {
		// mask only configured signal and event of type "Level"
		if (evt.event_code == glucose::NDevice_Event_Code::Level && evt.signal_id == mSignal_Id) {
			auto itr = mSegmentMaskState.find(evt.segment_id);
			if (itr == mSegmentMaskState.end())
				mSegmentMaskState[evt.segment_id] = 0;

			if (mMask.test(mSegmentMaskState[evt.segment_id]))
			{
				// This is certainly not nice, but we want to avoid creating a new device event, because it will generate new logical clock value, which would be
				// out of order with the rest of levels; also Level and Masked_Level uses the same union value, so it's fine to just change the type
				glucose::TDevice_Event* raw;
				if (evt.get()->Raw(&raw) == S_OK)
					raw->event_code = glucose::NDevice_Event_Code::Masked_Level;
			}

			mSegmentMaskState[evt.segment_id] = (mSegmentMaskState[evt.segment_id] + 1) % mBitCount;
		}

		if (!mOutput.Send(evt))
			break;
	}

	return S_OK;
};
