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


	if (!refcnt::Shared_Valid_All(mIst, mCOB, mIOB)) throw std::exception{"Cannot obtain all segment signals!"};
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

	auto identity = [](const auto Z, auto &A) { A = Z; };
	auto ReLU = [](const auto Z, auto &A) { A.array() = Z.array().max(0.0); };
	auto sigmoid = [](const auto Z, auto &A) { A.array() = 1.0/(1.0 + (-Z.array()).exp()); };

	auto activate = identity;
	const double final_threshold = 0.5;

	for (size_t i = 0; i < count; i++) {
		double iob, cob;
		std::array<double, 3> ist;
		std::array<double, 3> ist_times = { times[i] - parameters.dt - 10.0*scgms::One_Minute, 
			                                times[i] - parameters.dt - 5.0*scgms::One_Minute,
											times[i] - parameters.dt };
		
		bool data_ok = mIOB->Get_Continuous_Levels(nullptr, &ist_times[2], &iob, 1, scgms::apxNo_Derivation) == S_OK;
		data_ok &= mCOB->Get_Continuous_Levels(nullptr, &ist_times[2], &cob, 1, scgms::apxNo_Derivation) == S_OK;
		data_ok &= mIst->Get_Continuous_Levels(nullptr, ist_times.data(), ist.data(), ist.size(), scgms::apxNo_Derivation) == S_OK;
		data_ok &= !Is_NaN(iob, cob, ist[0], ist[1], ist[2]);

		if (data_ok) {
			input(0) = ist[0]; input(1) = ist[1]; input(2) = ist[2]; input(3) = iob; input(4) = cob;
			activate(l0 * input, i0);
			activate(l1 * i0,	i1);
			activate(l1 * i1,	i2);
			activate(l1 * i2,	i3);
			
			/*auto classes = i3.array();
			classes = classes.cwiseMin(1.0);
			classes = classes.cwiseMax(0.0);
			*/
			double sum = 0.0;
			double power = 1.0;
			for (size_t i = 0; i < TI3::RowsAtCompileTime; i++) {
				//sum += std::round(classes(i))*power;
				sum += power * (i3(i) >= final_threshold ? 1.0 : 0.0);
				power *= 2.0;
			}

			levels[i] = i3.sum();
		} else
			levels[i] = std::numeric_limits<double>::quiet_NaN();
	}

	return S_OK;
}

HRESULT IfaceCalling CConst_Neural_Net_Prediction_Signal::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(const_neural_net::default_parameters.vector.data());
	return parameters->set(params, params + const_neural_net::default_parameters.vector.size());	
}
