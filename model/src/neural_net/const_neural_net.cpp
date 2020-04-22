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
#include "../../../../common/rtl/Eigen_Buffer.h"
#include "../../../../common/utils/math_utils.h"


CConst_Neural_Net_Prediction_Signal::CConst_Neural_Net_Prediction_Signal(scgms::WTime_Segment segment) : 
	CCommon_Calculated_Signal(segment), 
	mIst(segment.Get_Signal(scgms::signal_IG)),
	mCOB(segment.Get_Signal(scgms::signal_COB)),
	mIOB(segment.Get_Signal(scgms::signal_IOB)) {


	if (!refcnt::Shared_Valid_All(mIst, mCOB, mIOB)) throw std::exception{};
}

HRESULT IfaceCalling CConst_Neural_Net_Prediction_Signal::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	if (derivation_order != scgms::apxNo_Derivation) return E_NOTIMPL;

	const const_neural_net::TParameters &parameters = scgms::Convert_Parameters<const_neural_net::TParameters>(params, const_neural_net::default_parameters.vector.data());

	TIn input;
	const TL0 l0 = Map_Double_To_Eigen<TL0>(parameters.weight0.data());
	const TL1 l1 = Map_Double_To_Eigen<TL1>(parameters.weight1.data());
	const TL2 l2 = Map_Double_To_Eigen<TL2>(parameters.weight2.data());
	const TL3 l3 = Map_Double_To_Eigen<TL3>(parameters.weight3.data());
	
	TI0 i0;
	TI1 i1;
	TI2 i2;
	TI3 i3;

	auto identity = [](const auto &Z, auto &A) { A = Z; };
	auto ReLU = [](const auto &Z, auto &A) { A.array() = Z.array().max(0.0); };
	auto sigmoid = [](const auto &Z, auto &A) { A.array() = 1.0/(1.0 + (-Z.array()).exp()); };

	auto activate = sigmoid;
	const double final_threshold = 0.5;

	for (size_t levels_idx = 0; levels_idx < count; levels_idx++) {
		double iob, cob;
		std::array<double, 3> ist;
		const double back_time = times[levels_idx] - parameters.dt;
		std::array<double, 3> ist_times = { back_time - 10.0*scgms::One_Minute,
											back_time - 5.0*scgms::One_Minute,
											back_time };
		
		bool data_ok = mIOB->Get_Continuous_Levels(nullptr, &ist_times[2], &iob, 1, scgms::apxNo_Derivation) == S_OK;
		data_ok &= mCOB->Get_Continuous_Levels(nullptr, &ist_times[2], &cob, 1, scgms::apxNo_Derivation) == S_OK;
		data_ok &= mIst->Get_Continuous_Levels(nullptr, ist_times.data(), ist.data(), ist.size(), scgms::apxNo_Derivation) == S_OK;
		data_ok &= !Is_NaN(iob, cob, ist[0], ist[1], ist[2]);

		if (data_ok) {
			auto dbl_2_dir = [](const double val) {
				if (val == 0.0) return 0.0;
				return val < 0.0 ? -1.0 : 1.0;
			};
			
			auto lev_2_idx = [](const double level) {
				double result = std::max(level, 12.0);
				result -= 2.0;
				result /= 10.0;
				return result / static_cast<double>(const_neural_net::layers_size[const_neural_net::layers_count-1]);
			};

			auto idx_2_level = [](const double idx) {
				double result = idx * static_cast<double>(const_neural_net::layers_size[const_neural_net::layers_count - 1]);
				result *= 10.0;
				result += 2.0;
				return result;
			};

			const double raw_x2 = 0.5*(ist[2] + ist[0]) - ist[1];
			const double raw_x = ist[1] - ist[0] - raw_x2;

			
			input(0) = dbl_2_dir(raw_x2);
			input(1) = dbl_2_dir(raw_x);
			input(2) = lev_2_idx(ist[2]);
					

			//input(0) = ist[0]; input(1) = ist[1]; input(2) = ist[2]; 
			input(3) = iob; input(4) = cob;
			activate(l0 * input, i0);
			activate(l1 * i0,	i1);
			activate(l1 * i1,	i2);
			activate(l1 * i2,	i3);

			i3.normalize();
			double sum = 0.0;
			const double area = i3.sum();
			
			if (area > 0.0) {
				const double base = 4.0;
				const double width = 4.0;

				double moments = 0.0;
				for (auto i = 0; i < TI3::RowsAtCompileTime; i++) {
					moments += i3(i)*static_cast<double>(i);
				}

				sum = moments / area;

				sum = idx_2_level(sum);
			}

			/*
			double sum = 0.0;
			double power = 1.0;
			for (size_t i3_idx = 0; i3_idx < TI3::RowsAtCompileTime; i3_idx++) {
				//sum += std::round(classes(i))*power;
			
				const double tmp = i3(i3_idx);
				if (std::isnormal(tmp)) 
					//sum += tmp*power;
					sum += power * (tmp >= final_threshold ? 1.0 : 0.0);
				power *= 2.0;
			}
			
			//sum *= max_level;
			*/
			levels[levels_idx] = sum != 0.0 ? sum : std::numeric_limits<double>::quiet_NaN();			
		} else
			levels[levels_idx] = std::numeric_limits<double>::quiet_NaN();
	}

	return S_OK;
}

HRESULT IfaceCalling CConst_Neural_Net_Prediction_Signal::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(const_neural_net::default_parameters.vector.data());
	return parameters->set(params, params + const_neural_net::default_parameters.vector.size());	
}
