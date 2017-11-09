#pragma once

#include "../../../common/iface/FilterIface.h"

#include <vector>



class CFilter_Factories : public std::vector<glucose::IFilter_Factory*> {
protected:

public:

};

extern CFilter_Factories filter_factories;

#ifdef _WIN32
	extern "C" __declspec(dllexport)  HRESULT IfaceCalling get_filter_factories(glucose::IFilter_Factory **begin, glucose::IFilter_Factory **end);
#endif