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

	HRESULT Create_Calculated_Signal(const GUID &calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) const {
		const auto iter = id_map.find(calc_id);
		if (iter != id_map.end())
			return iter->second(segment, signal);
		else return E_NOTIMPL;
	}
};



static CId_Dispatcher Id_Dispatcher;


HRESULT IfaceCalling do_create_calculated_signal(const GUID *calc_id, glucose::ITime_Segment *segment, glucose::ISignal **signal) {
	if ((calc_id ==nullptr) || (segment == nullptr)) return E_INVALIDARG;
	return Id_Dispatcher.Create_Calculated_Signal(*calc_id, segment, signal);
}