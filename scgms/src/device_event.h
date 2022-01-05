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

#pragma once

#include "../../../common/iface/DeviceIface.h"

class CDevice_Event : public virtual scgms::IDevice_Event {
protected:
	scgms::TDevice_Event mRaw;
	size_t mSlot = std::numeric_limits<size_t>::max();
	void Clean_Up() noexcept;
public:
	CDevice_Event() noexcept {};
	virtual ~CDevice_Event() noexcept;

	void Set_Slot(const size_t slot) noexcept { mSlot = slot; };

	void Initialize(const scgms::NDevice_Event_Code code) noexcept;
	void Initialize(const scgms::TDevice_Event* event) noexcept;


	virtual ULONG IfaceCalling Release() noexcept override;
	virtual HRESULT IfaceCalling Raw(scgms::TDevice_Event** dst) noexcept override;
	virtual HRESULT IfaceCalling Clone(IDevice_Event** event) noexcept override;

	//tiny helper for debugging
	size_t logical_clock() noexcept { return mRaw.logical_time; }
};


scgms::IDevice_Event* allocate_device_event(scgms::NDevice_Event_Code code) noexcept;