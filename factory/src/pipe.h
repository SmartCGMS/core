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

#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include <tbb/concurrent_queue.h>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CFilter_Pipe :  public glucose::IFilter_Pipe, public virtual refcnt::CReferenced {
protected:
	tbb::concurrent_bounded_queue<glucose::IDevice_Event*> mQueue;
	const std::ptrdiff_t mDefault_Capacity = 64;
	bool mShutting_Down_Send = false;
	bool mShutting_Down_Receive = false;
		//we need two flags to let the last shutdown event to fall through
public:
	CFilter_Pipe() noexcept;
	virtual ~CFilter_Pipe();

	virtual HRESULT send(glucose::IDevice_Event* event) override final;
	virtual HRESULT receive(glucose::IDevice_Event** event) override final;
	virtual HRESULT abort() final;
};

#pragma warning( pop )

#ifdef _WIN32
	extern "C" __declspec(dllexport) HRESULT IfaceCalling create_filter_pipe(glucose::IFilter_Pipe **pipe);
#else
	extern "C" HRESULT IfaceCalling create_filter_pipe(glucose::IFilter_Pipe **pipe);
#endif
