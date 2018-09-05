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

#include "device_event.h"

#include "../../../common/rtl/rattime.h"
#include "../../../common/rtl/manufactory.h"
#include "../../../common/rtl/referencedImpl.h"

#include <atomic>

std::atomic<int64_t> global_logical_time{ 0 };

CDevice_Event::CDevice_Event(glucose::NDevice_Event_Code code) {
	memset(&mRaw, 0, sizeof(mRaw));
	mRaw.logical_time = global_logical_time.fetch_add(1);
	mRaw.event_code = code;
	mRaw.device_time = Unix_Time_To_Rat_Time(time(nullptr));
	mRaw.segment_id = glucose::Invalid_Segment_Id;
	

	switch (code) {
		case glucose::NDevice_Event_Code::Information:
		case glucose::NDevice_Event_Code::Warning:
		case glucose::NDevice_Event_Code::Error:			mRaw.info = refcnt::WString_To_WChar_Container(nullptr);
			break;

		case glucose::NDevice_Event_Code::Parameters:
		case glucose::NDevice_Event_Code::Parameters_Hint:	mRaw.parameters = refcnt::Create_Container<double>(nullptr, nullptr);
			break;

		default:mRaw.level = std::numeric_limits<double>::quiet_NaN();
			break;
}
}

CDevice_Event::~CDevice_Event() {
	switch (mRaw.event_code) {
		case glucose::NDevice_Event_Code::Information:
		case glucose::NDevice_Event_Code::Warning:
		case glucose::NDevice_Event_Code::Error:			if (mRaw.info) mRaw.info->Release();
															break;

		case glucose::NDevice_Event_Code::Parameters:
		case glucose::NDevice_Event_Code::Parameters_Hint:	if (mRaw.parameters) mRaw.parameters->Release();
															break;
		default:	break;
	}

}

ULONG IfaceCalling CDevice_Event::Release() {
	delete this;
	return 0;
}

HRESULT IfaceCalling CDevice_Event::Raw(glucose::TDevice_Event **dst) {
	*dst = &mRaw;
	return S_OK;
}

HRESULT IfaceCalling create_device_event(glucose::NDevice_Event_Code code, glucose::IDevice_Event **event) {
	CDevice_Event *tmp = new CDevice_Event{code};
	*event = static_cast<glucose::IDevice_Event*> (tmp);
	return S_OK;
}
