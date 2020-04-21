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

#pragma once

#include "../../../../common/iface/DeviceIface.h"
#include "../../../../common/rtl/FilterLib.h"
#include "../../../../common/rtl/Common_Calculated_Signal.h"

#include "../descriptor.h"

#include <Eigen/Dense>


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CConst_Neural_Net_Prediction_Signal : public virtual CCommon_Calculated_Signal {
protected:
	using TIn = Eigen::Matrix<double, const_neural_net::input_count, 1>;
	using TL0 = Eigen::Matrix<double, const_neural_net::layers_size[0], const_neural_net::input_count>;
	using TL1 = Eigen::Matrix<double, const_neural_net::layers_size[1], const_neural_net::layers_size[0]>;
	using TL2 = Eigen::Matrix<double, const_neural_net::layers_size[2], const_neural_net::layers_size[1]>;
	using TL3 = Eigen::Matrix<double, const_neural_net::layers_size[3], const_neural_net::layers_size[2]>;

	using TI0 = Eigen::Matrix<double, const_neural_net::layers_size[0], 1>;
	using TI1 = Eigen::Matrix<double, const_neural_net::layers_size[1], 1>;
	using TI2 = Eigen::Matrix<double, const_neural_net::layers_size[2], 1>;
	using TI3 = Eigen::Matrix<double, const_neural_net::layers_size[3], 1>;
protected:
	scgms::SSignal mIst, mCOB, mIOB;
public:
	CConst_Neural_Net_Prediction_Signal(scgms::WTime_Segment segment);
	virtual ~CConst_Neural_Net_Prediction_Signal() = default;

	//scgms::ISignal iface
	virtual HRESULT IfaceCalling Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
		const double* times, double* const levels, const size_t count, const size_t derivation_order) const final;
	virtual HRESULT IfaceCalling Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const final;
};


#pragma warning( pop )