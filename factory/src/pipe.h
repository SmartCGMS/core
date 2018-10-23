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
