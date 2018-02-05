#include "db_reader.h"

#include "../../../common/rtl/FilterLib.h"

#include "../../../common/lang/dstrings.h"

namespace db_reader {

	constexpr size_t param_count = 6;
	constexpr glucose::NParameter_Type param_type[param_count] = { glucose::NParameter_Type::ptWChar_Container,  glucose::NParameter_Type::ptWChar_Container, glucose::NParameter_Type::ptWChar_Container,  glucose::NParameter_Type::ptWChar_Container,  glucose::NParameter_Type::ptWChar_Container, glucose::NParameter_Type::ptSelect_Time_Segment_ID };
	const wchar_t* ui_param_name[param_count] = { dsDb_Host, dsDb_Provider, dsDb_Name, dsDb_User_Name, rsDb_Password, dsTime_Segment_ID};
	const wchar_t* config_param_name[param_count] = { rsDb_Host, rsDb_Provider, rsDb_Name, rsDb_User_Name, rsDb_Password,  rsTime_Segment_ID };

	
	const glucose::TFilter_Descriptor Db_Reader_Descriptor = {
		{ 0xc0e942b9, 0x3928, 0x4b81,{ 0x9b, 0x43, 0xa3, 0x47, 0x66, 0x82, 0x0, 0x42 } },	//// {C0E942B9-3928-4B81-9B43-A34766820042}	
		dsDb_Reader,
		param_count,
		param_type,
		ui_param_name,
		config_param_name,
	};

	
}




HRESULT CDb_Reader::configure(const glucose::TFilter_Parameter *configuration, const size_t count) {
	return E_NOTIMPL;
};