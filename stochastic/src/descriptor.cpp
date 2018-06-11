#include "descriptor.h"
#include "../../../common/iface/DeviceIface.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/descriptor_utils.h"
#include <vector>

#include <tbb/tbb_allocator.h>

namespace newuoa {
	const glucose::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsNewUOA);
}


namespace metade {
	const glucose::TSolver_Descriptor desc = Describe_Non_Specialized_Solver(id, dsMetaDE);
}



const std::vector<glucose::TSolver_Descriptor, tbb::tbb_allocator<glucose::TSolver_Descriptor>> solver_descriptions = { newuoa::desc, metade::desc };




HRESULT IfaceCalling do_get_solver_descriptors(glucose::TSolver_Descriptor **begin, glucose::TSolver_Descriptor **end) {
	return do_get_descriptors(solver_descriptions, begin, end);
}