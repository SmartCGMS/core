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
 * Univerzitni 8
 * 301 00, Pilsen
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

#include "errors.h"

#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/descriptor_utils.h"

const std::array < glucose::TMetric_Descriptor, 10 > metric_descriptor = { {
	 glucose::TMetric_Descriptor{ mtrAvg_Abs, dsAvg_Abs },
	 glucose::TMetric_Descriptor{ mtrMax_Abs, dsMax_Abs },
	 glucose::TMetric_Descriptor{ mtrPerc_Abs, dsPerc_Abs },
	 glucose::TMetric_Descriptor{ mtrThresh_Abs, dsThresh_Abs },
	 glucose::TMetric_Descriptor{ mtrLeal_2010, dsLeal_2010 },
	 glucose::TMetric_Descriptor{ mtrAIC, dsAIC },
	 glucose::TMetric_Descriptor{ mtrStd_Dev, dsStd_Dev },
	 glucose::TMetric_Descriptor{ mtrCrosswalk, dsCrosswalk },
	 glucose::TMetric_Descriptor{ mtrIntegral_CDF, dsIntegral_CDF },
	 glucose::TMetric_Descriptor{ mtrAvg_Plus_Bessel_Std_Dev, dsAvg_Plus_Bessel_Std_Dev }
} };

HRESULT IfaceCalling do_get_metric_descriptors(glucose::TMetric_Descriptor **begin, glucose::TMetric_Descriptor **end) {
	*begin = const_cast<glucose::TMetric_Descriptor*>(metric_descriptor.data());
	*end = *begin + metric_descriptor.size();
	return S_OK;
}

namespace errors
{
	constexpr size_t param_count = 0;

	const glucose::TFilter_Descriptor Errors_Descriptor = {
		glucose::Error_Filter,
		glucose::NFilter_Flags::Synchronous,
		dsErrors_Filter,
		param_count,
		nullptr,
		nullptr,
		nullptr,
		nullptr
	};
}

static const std::array<glucose::TFilter_Descriptor, 1> filter_descriptions = { { errors::Errors_Descriptor } };

extern "C" HRESULT IfaceCalling do_get_filter_descriptors(glucose::TFilter_Descriptor **begin, glucose::TFilter_Descriptor **end) {
	*begin = const_cast<glucose::TFilter_Descriptor*>(filter_descriptions.data());
	*end = *begin + filter_descriptions.size();
	return S_OK;
}

extern "C" HRESULT IfaceCalling do_create_filter(const GUID *id, glucose::IEvent_Receiver *input, glucose::IEvent_Sender *output, glucose::IFilter **filter) {
	auto shared_input = refcnt::make_shared_reference_ext<glucose::SEvent_Receiver, glucose::IEvent_Receiver>(input, true);
	auto shared_output = refcnt::make_shared_reference_ext<glucose::SEvent_Sender, glucose::IEvent_Sender>(output, true);


	if (*id == errors::Errors_Descriptor.id)
		return Manufacture_Object<CErrors_Filter>(filter, shared_input, shared_output);

	return ENOENT;
}
