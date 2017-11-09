#include "filters.h"


CFilter_Factories filter_factories;


HRESULT IfaceCalling get_filter_factories(glucose::IFilter_Factory **begin, glucose::IFilter_Factory **end) {

	if (!filter_factories.empty()) {
		*begin = filter_factories[0];
		*end = *begin + filter_factories.size();
	}
	else
		*begin = *end = nullptr;

	

	return S_OK;
}

