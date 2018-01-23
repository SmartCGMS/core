#pragma once

#include "descriptor.h"
#include "db_reader.h"

#include <vector>

const std::vector<glucose::TFilter_Descriptor> filter_descriptions = { db_reader::Db_Reader_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}