#pragma once

#include "descriptor.h"
#include "db_reader.h"
#include "../../../common/rtl/descriptor_utils.h"

#include <vector>

const std::vector<glucose::TFilter_Descriptor> filter_descriptions = { db_reader::Db_Reader_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	return do_get_descriptors(filter_descriptions, begin, end);
}