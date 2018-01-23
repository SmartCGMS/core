#pragma once

#include "../../../common/iface/FilterIface.h"
#include "../../../common/iface/UIIface.h"

namespace db_reader {
	extern const glucose::TFilter_Descriptor Db_Reader_Descriptor;
}

class CDb_Reader : public glucose::IFilter {
	//class that reads selected segments from the db produces the events
	//i.e., it mimicks CGMS
protected:
public:
	virtual HRESULT configure(const glucose::TFilter_Parameter *configuration, const size_t count) final;
};