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

#include "factory.h"

#include "Diffusion_v2_blood.h"
#include "Diffusion_v2_ist.h"
#include "Steil_Rebrin_blood.h"
#include "descriptor.h"

#include <map>
#include <functional>


#include "../../../common/rtl/manufactory.h"
#include "../../../common/rtl/DeviceLib.h"

#include <tbb/tbb_allocator.h>

using TCreate_Signal = std::function<HRESULT(glucose::ITime_Segment *segment, glucose::ISignal **signal)>;
using TAlloc = tbb::tbb_allocator<std::pair<const GUID, TCreate_Signal>>;

class CId_Dispatcher {
protected:
	std::map <const GUID, TCreate_Signal, std::less<GUID>, TAlloc> id_map;

	template <typename T>
	HRESULT Create_X(glucose::ITime_Segment *segment, glucose::ISignal **signal) const {
		glucose::WTime_Segment weak_segment{ segment };
		return Manufacture_Object<T, glucose::ISignal>(signal, weak_segment);
	}

	template <typename T>
	void Add_Signal(const GUID &id) {
		id_map[id] = std::bind(&CId_Dispatcher::Create_X<T>, this, std::placeholders::_1, std::placeholders::_2);
	}
public:
	CId_Dispatcher() {
		Add_Signal<CDiffusion_v2_blood>(glucose::signal_Diffusion_v2_Blood);
		Add_Signal<CDiffusion_v2_ist>(glucose::signal_Diffusion_v2_Ist);
		Add_Signal<CSteil_Rebrin_blood>(glucose::signal_Steil_Rebrin_Blood);
	}

	HRESULT Create_Signal(const GUID &calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) const {
		const auto iter = id_map.find(calc_id);
		if (iter != id_map.end())
			return iter->second(segment, signal);
		else
			return E_NOTIMPL;
	}
};

static CId_Dispatcher Id_Dispatcher;

extern "C" HRESULT IfaceCalling do_create_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) {
	if ((calc_id ==nullptr) || (segment == nullptr)) return E_INVALIDARG;
	return Id_Dispatcher.Create_Signal(*calc_id, segment, signal);
}
