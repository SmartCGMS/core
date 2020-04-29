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

#include "const_neural_net.h"

#include "../../../../common/rtl/SolverLib.h"
#include "../../../../common/utils/math_utils.h"


CConst_Neural_Net_Prediction_Signal::CConst_Neural_Net_Prediction_Signal(scgms::WTime_Segment segment) : 
	CCommon_Calculated_Signal(segment), 
	mIst(segment.Get_Signal(scgms::signal_IG)),
	mCOB(segment.Get_Signal(scgms::signal_COB)),
	mIOB(segment.Get_Signal(scgms::signal_IOB)) {

	if (!refcnt::Shared_Valid_All(mIst, mCOB, mIOB)) 
		throw std::runtime_error{ "Cannot obtain all segment signals!" };
}

HRESULT IfaceCalling CConst_Neural_Net_Prediction_Signal::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	if (derivation_order != scgms::apxNo_Derivation) return E_NOTIMPL;
	const_neural_net::CNeural_Network neural_net;
	const const_neural_net::TParameters& parameters = scgms::Convert_Parameters<const_neural_net::TParameters>(params, const_neural_net::default_parameters.vector.data());
	neural_net.Initialize(parameters.neural_parameters.data(), parameters.neural_parameters.size());	

	decltype(neural_net)::TInput input;
	for (size_t levels_idx = 0; levels_idx < count; levels_idx++) {
		std::array<double, 3> ist, iob, cob;
		const double back_time = times[levels_idx] - parameters.dt;
		std::array<double, 3> ist_times = { back_time - 10.0 * scgms::One_Minute,
											back_time - 5.0 * scgms::One_Minute,
											back_time };

		bool data_ok = mIOB->Get_Continuous_Levels(nullptr, ist_times.data(), iob.data(), iob.size(), scgms::apxNo_Derivation) == S_OK;
		data_ok &= mCOB->Get_Continuous_Levels(nullptr, ist_times.data(), cob.data(), cob.size(), scgms::apxNo_Derivation) == S_OK;
		data_ok &= mIst->Get_Continuous_Levels(nullptr, ist_times.data(), ist.data(), ist.size(), scgms::apxNo_Derivation) == S_OK;
		data_ok &= !Is_NaN(iob, cob, ist);

		if (data_ok) {
			const double x_raw = levels[2] - levels[0];
			double x2_raw = -1.0;

			if (x_raw != 0.0) {
				const double d1 = fabs(levels[1] - levels[0]);
				const double d2 = fabs(levels[2] - levels[1]);

				//if ((d1 != 0.0) && (d2 > d1)) x2_raw = 1.0;	-- the ifs below are more accurate

				const bool acc = d2 > d1;
				if (acc) {
					if (x_raw > 0.0) {
						if ((levels[1] > levels[0]) && (levels[2] > levels[1])) x2_raw = 1.0;
					}

					if (x_raw < 0.0) {
						if ((levels[1] < levels[0]) && (levels[2] < levels[1])) x2_raw = 1.0;
					}
				}
			}

			if (levels[2] != levels[0]) input(0) = levels[2] - levels[0] > 0.0 ? 1.0 : -1.0;
			else input(0) = 0.0;

			input(1) = x2_raw;

			input(2) = neural_net::Level_To_Histogram_Index(ist[2]) - static_cast<double>(neural_net::Band_Count) * 0.5;
			input(2) /= static_cast<double>(neural_net::Band_Count) * 0.5;
			//input(2) /= (std::min(16.0, ist[2]) - 8.0) / 8.0;

			input(3) = iob[2] - iob[0] > 0.0 ? 1.0 : -1.0;
			input(4) = cob[2] - cob[0] > 0.0 ? 1.0 : -1.0;
			
			//levels[levels_idx] = neural_net::Histogram_Index_To_Level(neural_net.Forward(input));
			levels[levels_idx] = static_cast<double>(neural_net.Forward(input).to_ulong())*0.5;

		} else
			levels[levels_idx] = std::numeric_limits<double>::quiet_NaN();
	}

	return S_OK;
}

HRESULT IfaceCalling CConst_Neural_Net_Prediction_Signal::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(const_neural_net::default_parameters.vector.data());
	return parameters->set(params, params + const_neural_net::default_parameters.vector.size());	
}
