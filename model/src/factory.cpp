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
		else return E_NOTIMPL;
	}
};



static CId_Dispatcher Id_Dispatcher;


HRESULT IfaceCalling do_create_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) {
	if ((calc_id ==nullptr) || (segment == nullptr)) return E_INVALIDARG;
	return Id_Dispatcher.Create_Signal(*calc_id, segment, signal);
}