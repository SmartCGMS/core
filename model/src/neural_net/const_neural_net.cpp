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

double Level_To_Histogram_Index(const double level) {
	if (level >= const_neural_net::High_Threshold) return static_cast<double>(const_neural_net::Band_Count - 1);

	const double tmp = level - const_neural_net::Low_Threshold;
	if (tmp < 0.0) return 0.0;

	return floor(tmp * const_neural_net::Inv_Band_Size);
}

double Histogram_Index_To_Level(double index) {
	index = floor(index);
	if (index == 0.0) return const_neural_net::Low_Threshold - const_neural_net::Half_Band_Size;
	if (index >= const_neural_net::Band_Count - 1) return const_neural_net::High_Threshold + const_neural_net::Half_Band_Size;

	return const_neural_net::Low_Threshold + static_cast<double>(index - 1) * const_neural_net::Band_Size + const_neural_net::Half_Band_Size;
}


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

	const const_neural_net::TParameters &parameters = scgms::Convert_Parameters<const_neural_net::TParameters>(params, const_neural_net::default_parameters.vector.data());

	TA_In input;
	const TW_H1 w_h1 = Map_Double_To_Eigen<TW_H1>(parameters.weight_input_to_hidden1.data());
	const TW_H2 w_h2 = Map_Double_To_Eigen<TW_H2>(parameters.weight_hidden1_to_hidden2.data());
	const TW_Out w_out = Map_Double_To_Eigen<TW_Out>(parameters.weight_hidden2_to_output.data());
	
	TA_H1 a_h1;
	TA_H2 a_h2;
	TA_Out a_out;
	

	//https://github.com/yixuan/MiniDNN
	auto identity = [](const auto &Z, auto &A) { A = Z; };
	auto ReLU = [](const auto &Z, auto &A) { A.array() = Z.array().max(0.0); };
	auto tanh = [](const auto& Z, auto& A) { A.array() = Z.array().tanh(); };
	auto sigmoid = [](const auto &Z, auto &A) { A.array() = 1.0/(1.0 + (-Z.array()).exp()); };
	auto soft_max = [this](const auto& Z, auto& A) { 
		soft_max_t<std::remove_reference_t<decltype(Z)>, std::remove_reference_t<decltype(A)>>(Z, A);			
	};

	auto activate = tanh;
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
			
			auto cent_lev = [](const double lev) {
				double result = Level_To_Histogram_Index(lev) - 3.0;// static_cast<double>(const_neural_net::Band_Count) * 0.5;
				return result * 0.5;
			};

			const double raw_x2 = 0.5*(ist[2] + ist[0]) - ist[1];
			const double raw_x = ist[1] - ist[0] - raw_x2;

			
			input(0) = dbl_2_dir(raw_x2);
			input(1) = dbl_2_dir(raw_x);
			
			//input(2) = Level_To_Histogram_Index(ist[2])-static_cast<double>(const_neural_net::Band_Count)*0.5;
			
			input(2) = cent_lev(ist[2]);
			input(3) = cent_lev(iob);
			input(4) = cent_lev(cob);

			//input(0) = ist[0]; input(1) = ist[1]; input(2) = ist[2]; 
			//input(3) = iob; input(4) = cob;

			activate(w_h1 * input, a_h1);
			activate(w_h2 * a_h1, a_h2);
			soft_max(w_out * a_h2, a_out);
			

			size_t best_idx = 0;
			double best_val = a_out(0);

			
			for (auto i = 1; i < TA_Out::RowsAtCompileTime; i++) {
				if (a_out(i) > best_val) {
					best_val = a_out(i);
					best_idx = i;
				}
			}

			levels[levels_idx] = Histogram_Index_To_Level(static_cast<double>(best_idx));
		} else
			levels[levels_idx] = std::numeric_limits<double>::quiet_NaN();
	}

	return S_OK;
}

HRESULT IfaceCalling CConst_Neural_Net_Prediction_Signal::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(const_neural_net::default_parameters.vector.data());
	return parameters->set(params, params + const_neural_net::default_parameters.vector.size());	
}
