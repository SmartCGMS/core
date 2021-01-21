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
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "descriptor.h"
#include "db_reader.h"
#include "db_writer.h"
#include "file_reader.h"
#include "sincos_generator.h"
#include "../../../common/utils/descriptor_utils.h"
#include "../../../common/iface/EmbeddedIface.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/manufactory.h"


#include <vector>

namespace db_reader {

	constexpr size_t param_count = 8;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptInt64,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptInt64_Array,
		scgms::NParameter_Type::ptBool
	};

	const wchar_t* ui_param_name[param_count] = {
		dsDb_Host,
		dsDb_Port,
		dsDb_Provider,
		dsDb_Name,
		dsDb_User_Name,
		dsDb_Password,
		dsTime_Segment_ID,
		dsShutdown_After_Last
	};

	const wchar_t* config_param_name[param_count] = {
		rsDb_Host,
		rsDb_Port,
		rsDb_Provider,
		rsDb_Name,
		rsDb_User_Name,
		rsDb_Password,
		rsTime_Segment_ID,
		rsShutdown_After_Last
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsDb_Host_Tooltip,
		dsDb_Port_Tooltip,
		dsDb_Provider_Tooltip,
		dsDb_Name_Tooltip,
		dsDb_Username_Tooltip,
		dsDb_Password_Tooltip,
		nullptr,
		dsShutdown_After_Last_Tooltip
	};

	const scgms::TFilter_Descriptor Db_Reader_Descriptor = {
		filter_id,
		scgms::NFilter_Flags::None,
		dsDb_Reader,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};

}

namespace db_writer {

	constexpr size_t param_count = 10;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptInt64,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptSubject_Id
	};

	const wchar_t* ui_param_name[param_count] = {
		dsDb_Host,
		dsDb_Port,
		dsDb_Provider,
		dsDb_Name,
		dsDb_User_Name,
		dsDb_Password,
		dsGenerate_Primary_Keys,
		dsStore_Data,
		dsStore_Parameters,
		dsSubject_Id
	};

	const wchar_t* config_param_name[param_count] = {
		rsDb_Host,
		rsDb_Port,
		rsDb_Provider,
		rsDb_Name,
		rsDb_User_Name,
		rsDb_Password,
		rsGenerate_Primary_Keys,
		rsStore_Data,
		rsStore_Parameters,
		rsSubject_Id
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsDb_Host_Tooltip,
		dsDb_Port_Tooltip,
		dsDb_Provider_Tooltip,
		dsDb_Name_Tooltip,
		dsDb_Username_Tooltip,
		dsDb_Password_Tooltip,
		dsGenerate_Primary_Keys_Tooltip,
		dsStore_Data_Tooltip,
		dsStore_Parameters_Tooltip,
		nullptr
	};

	const scgms::TFilter_Descriptor Db_Writer_Descriptor = {
		filter_id,
		scgms::NFilter_Flags::None,
		dsDb_Writer,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};

}


namespace file_reader
{
	constexpr size_t param_count = 5;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptWChar_Array,
		scgms::NParameter_Type::ptInt64,
		scgms::NParameter_Type::ptBool,
		scgms::NParameter_Type::ptInt64,
		scgms::NParameter_Type::ptBool,
	};

	const wchar_t* ui_param_name[param_count] = {
		dsInput_Values_File,
		dsInput_Segment_Spacing,
		dsShutdown_After_Last,
		dsMinimum_Segment_Levels,
		dsRequire_IG_BG
	};

	const wchar_t* config_param_name[param_count] = {
		rsInput_Values_File,
		rsInput_Segment_Spacing,
		rsShutdown_After_Last,
		rsMinimum_Segment_Levels,
		rsRequire_IG_BG
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		dsInput_Values_File_Tooltip,
		dsInput_Segment_Spacing_Tooltip,
		dsShutdown_After_Last_Tooltip,
		dsMinimum_Segment_Levels_Tooltip,
		dsRequire_IG_BG_Tooltip
	};

	const scgms::TFilter_Descriptor File_Reader_Descriptor = {
		{ 0xc0e942b9, 0x3928, 0x4b81,{ 0x9b, 0x43, 0xa3, 0x47, 0x66, 0x82, 0x0, 0xBB } }, //// {C0E942B9-3928-4B81-9B43-A347668200BB}
		scgms::NFilter_Flags::None,
		dsFile_Reader,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};
}

namespace sincos_generator {

	constexpr size_t param_count = 10;

	constexpr scgms::NParameter_Type param_type[param_count] = {
		scgms::NParameter_Type::ptDouble,
		scgms::NParameter_Type::ptDouble,
		scgms::NParameter_Type::ptRatTime,
		scgms::NParameter_Type::ptRatTime,
		scgms::NParameter_Type::ptDouble,
		scgms::NParameter_Type::ptDouble,
		scgms::NParameter_Type::ptRatTime,
		scgms::NParameter_Type::ptRatTime,
		scgms::NParameter_Type::ptRatTime,
		scgms::NParameter_Type::ptBool
	};

	const wchar_t* ui_param_name[param_count] = {
		dsGen_IG_Offset,
		dsGen_IG_Amplitude,
		dsGen_IG_Sin_Period,
		dsGen_IG_Sampling_Period,
		dsGen_BG_Offset,
		dsGen_BG_Amplitude,
		dsGen_BG_Cos_Period,
		dsGen_BG_Sampling_Period,
		dsGen_Total_Time,
		dsShutdown_After_Last
	};

	const wchar_t* config_param_name[param_count] = {
		rsGen_IG_Offset,
		rsGen_IG_Amplitude,
		rsGen_IG_Sin_Period,
		rsGen_IG_Sampling_Period,
		rsGen_BG_Level_Offset,
		rsGen_BG_Amplitude,
		rsGen_BG_Cos_Period,
		rsGen_BG_Sampling_Period,
		rsGen_Total_Time,
		rsShutdown_After_Last
	};

	const wchar_t* ui_param_tooltips[param_count] = {
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		dsShutdown_After_Last_Tooltip
	};

	const scgms::TFilter_Descriptor SinCos_Generator_Descriptor = {
		filter_id,
		scgms::NFilter_Flags::None,
		dsSinCos_Generator,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
		ui_param_tooltips
	};

}

static const std::array<scgms::TFilter_Descriptor, 4> filter_descriptions = { db_reader::Db_Reader_Descriptor, db_writer::Db_Writer_Descriptor, file_reader::File_Reader_Descriptor, sincos_generator::SinCos_Generator_Descriptor };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(scgms::TFilter_Descriptor **begin, scgms::TFilter_Descriptor **end) {
	return do_get_descriptors(filter_descriptions, begin, end);
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, scgms::IFilter *next_filter, scgms::IFilter **filter) {
	TEmbedded_Error err{ E_NOTIMPL, nullptr };


	if (*id == db_reader::Db_Reader_Descriptor.id)
		err = Manufacture_Object<CDb_Reader>(filter, next_filter);
	if (*id == db_writer::Db_Writer_Descriptor.id)
		err = Manufacture_Object<CDb_Writer>(filter, next_filter);
	else if (*id == file_reader::File_Reader_Descriptor.id)
		err = Manufacture_Object<CFile_Reader>(filter, next_filter);
	else if (*id == sincos_generator::SinCos_Generator_Descriptor.id)
		err = Manufacture_Object<CSinCos_Generator>(filter, next_filter);

	return err.code;
}
