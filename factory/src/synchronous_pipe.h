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
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/iface/FilterIface.h"
#include "../../../common/rtl/FilterLib.h"

#include <tbb/concurrent_queue.h>
#include <mutex>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CFilter_Synchronous_Pipe_RW : public virtual glucose::IFilter_Pipe_Reader, public virtual glucose::IFilter_Pipe_Writer, public virtual refcnt::CNotReferenced {
protected:
	// vector of events - initially, "send" pushes single event, but any synchronous filter may want to add more		
	refcnt::SVector_Container<glucose::IDevice_Event*> mFresh_Events, mDone_Events;
public:
	CFilter_Synchronous_Pipe_RW() {
		mFresh_Events = refcnt::Create_Container_shared<glucose::IDevice_Event*>(nullptr, nullptr);
		mDone_Events = refcnt::Create_Container_shared<glucose::IDevice_Event*>(nullptr, nullptr);
	}

	void push_fresh_event(glucose::IDevice_Event* event) {
		mFresh_Events->add(&event, &event + 1);	//since now, event is nullptr as we do not own it anymore
	}

	bool pop_done_event(glucose::IDevice_Event** event) {
		return mDone_Events->pop(event) == S_OK;
	}

	virtual HRESULT send(glucose::IDevice_Event* event) override final {
		return mDone_Events->add(&event, &event + 1);	//since now, event is nullptr as we do not own it anymore
	}
	
	virtual HRESULT receive(glucose::IDevice_Event** event) override final {
		return mFresh_Events->pop(event);
	}

	void Commit_Done_As_Fresh() {

	}
};

class CFilter_Synchronous_Pipe :  public virtual glucose::IFilter_Synchronous_Pipe, public virtual refcnt::CReferenced {
	protected:
		tbb::concurrent_bounded_queue<glucose::IDevice_Event*> mQueue;
		const std::ptrdiff_t mDefault_Capacity = 64;

		
		bool mShutting_Down_Send = false;
		bool mShutting_Down_Receive = false;

		// managed synchronous filters
		std::vector<glucose::SFilter> mFilters;
		// mutex for incoming events to protect Send method
		std::mutex mInMutex; 


		CFilter_Synchronous_Pipe_RW mRW;
	public:
		CFilter_Synchronous_Pipe() noexcept;
		virtual ~CFilter_Synchronous_Pipe() = default;

		// glucose::IFilter_Pipe iface
		virtual HRESULT send(glucose::IDevice_Event* event) override final;
		virtual HRESULT receive(glucose::IDevice_Event** event) override final;
		virtual HRESULT abort() final;

		// glucose::IFilter_Synchronous_Pipe iface
		virtual HRESULT add_filter(glucose::IFilter* synchronous_filter) override;

		virtual IFilter_Pipe_Reader* Get_Reader() override final {
			return dynamic_cast<IFilter_Pipe_Reader*>(&mRW);
		}

		virtual IFilter_Pipe_Writer* Get_Writer() override final {
			return dynamic_cast<IFilter_Pipe_Writer*>(&mRW);
		}
};

#pragma warning( pop )

#ifdef _WIN32
	extern "C" __declspec(dllexport) HRESULT IfaceCalling create_filter_synchronous_pipe(glucose::IFilter_Synchronous_Pipe **pipe);
#else
	extern "C" HRESULT IfaceCalling create_filter_synchronous_pipe(glucose::IFilter_Synchronous_Pipe **pipe);
#endif
