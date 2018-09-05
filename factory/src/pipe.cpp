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

#include "pipe.h"

#include "../../../common/rtl/manufactory.h"

HRESULT IfaceCalling create_filter_pipe(glucose::IFilter_Pipe **pipe) {
	return Manufacture_Object<CFilter_Pipe, glucose::IFilter_Pipe>(pipe);
}

CFilter_Pipe::CFilter_Pipe() noexcept {
	mQueue.set_capacity(mDefault_Capacity);
}

CFilter_Pipe::~CFilter_Pipe() {
	//just empty to call all the dctors correctly
}

HRESULT CFilter_Pipe::send(glucose::IDevice_Event* event) {
	if (mShutting_Down_Send)
		return S_FALSE;

	glucose::TDevice_Event *raw_event;
	HRESULT rc = event->Raw(&raw_event);
	if (rc != S_OK) return rc;
	
	if (raw_event->event_code == glucose::NDevice_Event_Code::Shut_Down)
		mShutting_Down_Send = true;

	try {
		mQueue.push(event);
	}
	catch (tbb::user_abort &) {
		mShutting_Down_Send = true;
		return S_FALSE;
	}
	return S_OK;
}

HRESULT CFilter_Pipe::receive(glucose::IDevice_Event** event) {

	if (mShutting_Down_Receive)
		return S_FALSE;

	try {
		mQueue.pop(*event);

		glucose::TDevice_Event *raw_event;
		if ((*event)->Raw(&raw_event) == S_OK)	//should be OK since we call it in ::send
			mShutting_Down_Receive = raw_event->event_code == glucose::NDevice_Event_Code::Shut_Down;
	}

	catch (tbb::user_abort &) {
		mShutting_Down_Receive = true;
		return S_FALSE;
	}
	return S_OK;
}

HRESULT CFilter_Pipe::abort() {
	//TODO: https://stackoverflow.com/questions/16961990/thread-building-blocks-concurrent-bounded-queue-how-do-i-close-it?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
	mShutting_Down_Send = mShutting_Down_Receive = true;
	mQueue.abort();
	return S_OK;
}
