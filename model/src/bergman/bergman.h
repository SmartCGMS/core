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

#include "descriptor.h"
#include "../../../common/rtl/FilterLib.h"

#include <thread>
#include <mutex>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * T1DMS device driver (pump setting to, and data source from T1DMS simulator)
 */
class CBergman_Device_Driver : public glucose::IDevice_Driver, public virtual refcnt::CReferenced
{
	private:
		glucose::SFilter_Pipe mInput;
		glucose::SFilter_Pipe mOutput;

		std::unique_ptr<CFeedback_Receiver_Base> mFeedback;

	protected:
		void Emit_Signal_Level(const GUID& id, double device_time, double level);
		void Emit_Sync_Request(double device_time);

	public:
		CBergman_Device_Driver(glucose::SFilter_Pipe& output);
		virtual ~CBergman_Device_Driver();

		// glucose::IAsynchronnous_Filter (glucose::IDevice_Driver) iface
		virtual HRESULT IfaceCalling Run(glucose::IFilter_Configuration* const configuration) override;

		// CNetwork_Feedback_Receiver iface
		//virtual void Handle_Synchronized(glucose::UDevice_Event& evt) override;
};

#pragma warning( pop )
