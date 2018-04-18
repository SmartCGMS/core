#pragma once

#include "descriptor.h"
#include "db_reader.h"
#include "file_reader.h"
#include "hold.h"
#include "../../../common/rtl/descriptor_utils.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"

#include <vector>

namespace db_reader {

	constexpr size_t param_count = 7;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptInt64,
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptSelect_Time_Segment_ID
	};

	const wchar_t* ui_param_name[param_count] = {
		dsDb_Host,
		dsDb_Port,
		dsDb_Provider,
		dsDb_Name,
		dsDb_User_Name,
		dsDb_Password,
		dsTime_Segment_ID
	};

	const wchar_t* config_param_name[param_count] = {
		rsDb_Host,
		rsDb_Port,
		rsDb_Provider,
		rsDb_Name,
		rsDb_User_Name,
		rsDb_Password,
		rsTime_Segment_ID
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};

	const glucose::TFilter_Descriptor Db_Reader_Descriptor = {
		{ 0xc0e942b9, 0x3928, 0x4b81,{ 0x9b, 0x43, 0xa3, 0x47, 0x66, 0x82, 0x0, 0x42 } }, //// {C0E942B9-3928-4B81-9B43-A34766820042}
		dsDb_Reader,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};

}

namespace file_reader
{
	constexpr size_t param_count = 2;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptWChar_Container,
		glucose::NParameter_Type::ptInt64
	};

	const wchar_t* ui_param_name[param_count] = {
		dsInput_Values_File,
		dsInput_Segment_Spacing
	};

	const wchar_t* config_param_name[param_count] = {
		rsInput_Values_File,
		rsInput_Segment_Spacing
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		nullptr
	};

	const glucose::TFilter_Descriptor File_Reader_Descriptor = {
		{ 0xc0e942b9, 0x3928, 0x4b81,{ 0x9b, 0x43, 0xa3, 0x47, 0x66, 0x82, 0x0, 0xBB } }, //// {C0E942B9-3928-4B81-9B43-A347668200BB}
		dsFile_Reader,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

namespace hold
{
	constexpr size_t param_count = 1;

	constexpr glucose::NParameter_Type param_type[param_count] = {
		glucose::NParameter_Type::ptInt64
	};

	const wchar_t* ui_param_name[param_count] = {
		dsHold_Values_Delay
	};

	const wchar_t* config_param_name[param_count] = {
		rsHold_Values_Delay
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr
	};

	const glucose::TFilter_Descriptor Hold_Descriptor = {
		{ 0xc0e942b9, 0x3928, 0x4b81,{ 0x9b, 0x43, 0xa3, 0x47, 0x66, 0x82, 0x0, 0xAA } }, //// {C0E942B9-3928-4B81-9B43-A347668200AA}
		dsHold_Filter,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

static const std::vector<glucose::TFilter_Descriptor> filter_descriptions = { db_reader::Db_Reader_Descriptor, file_reader::File_Reader_Descriptor, hold::Hold_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	return do_get_descriptors(filter_descriptions, begin, end);
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IFilter_Pipe *input, glucose::IFilter_Pipe *output, glucose::IFilter **filter)
{
	if (*id == db_reader::Db_Reader_Descriptor.id)
		return Manufacture_Object<CDb_Reader>(filter, input, output);
	else if (*id == file_reader::File_Reader_Descriptor.id)
		return Manufacture_Object<CFile_Reader>(filter, input, output);
	else if (*id == hold::Hold_Descriptor.id)
		return Manufacture_Object<CHold_Filter>(filter, input, output);

	return ENOENT;
}
