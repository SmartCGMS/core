#pragma once

#include "descriptor.h"
#include "db_reader.h"
#include "../../../common/rtl/descriptor_utils.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

static const std::vector<glucose::TFilter_Descriptor> filter_descriptions = { db_reader::Db_Reader_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	return do_get_descriptors(filter_descriptions, begin, end);
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == db_reader::Db_Reader_Descriptor.id)
		return Manufacture_Object<CDb_Reader>(filter, input, output);

	return ENOENT;
}
