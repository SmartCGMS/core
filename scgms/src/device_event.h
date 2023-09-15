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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include <scgms/iface/DeviceIface.h>

class CDevice_Event : public virtual scgms::IDevice_Event {
protected:
	scgms::TDevice_Event mRaw;
	size_t mSlot = std::numeric_limits<size_t>::max();
	void Clean_Up() noexcept;
public:
	CDevice_Event() noexcept {};
	CDevice_Event(CDevice_Event &&other) noexcept;
	virtual ~CDevice_Event() noexcept;

	void Set_Slot(const size_t slot) noexcept { mSlot = slot; };

	void Initialize(const scgms::NDevice_Event_Code code) noexcept;
	void Initialize(const scgms::TDevice_Event* event) noexcept;
	scgms::TDevice_Event& Raw() { return mRaw; };

	virtual ULONG IfaceCalling Release() noexcept override;
	virtual HRESULT IfaceCalling Raw(scgms::TDevice_Event** dst) noexcept override;
	virtual HRESULT IfaceCalling Clone(IDevice_Event** event) const noexcept override;

	//tiny helper for debugging
	size_t logical_clock() noexcept { return mRaw.logical_time; }
};


scgms::IDevice_Event* allocate_device_event(scgms::NDevice_Event_Code code) noexcept;