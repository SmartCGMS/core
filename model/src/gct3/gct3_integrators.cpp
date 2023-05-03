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
 * a) Without a specific agreement, you are not authorized to make or keep any copies of this file.
 * b) For any use, especially commercial use, you must contact us and obtain specific terms and conditions 
 *    for the use of the software.
 * c) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#include "gct3_integrators.h"
#include "gct3_transfer_functions.h"

namespace gct3_model {

	double CRectangular_Integrator::Integrate(double time_start, double time_end) const {
		const double y_diff = time_end - time_start;
		const double x0 = mFunction.Calculate_Transfer_Input(time_start);

		return y_diff * x0;
	}

	double CMidpoint_Integrator::Integrate(double time_start, double time_end) const {
		const double y_diff = time_end - time_start;
		const double x0 = mFunction.Calculate_Transfer_Input(time_start);
		const double x1 = mFunction.Calculate_Transfer_Input(time_end);

		return y_diff * (x1 + x0) * 0.5;
	}

	double CSimpson_1_3_Rule_Integrator::Integrate(double time_start, double time_end) const {
		const double y_diff = time_end - time_start;
		const double x0 = mFunction.Calculate_Transfer_Input(time_start);
		const double x1 = mFunction.Calculate_Transfer_Input((time_start + time_end) / 2.0);
		const double x2 = mFunction.Calculate_Transfer_Input(time_end);

		return (y_diff / 6.0) * (x0 + 4 * x1 + x2);
	}

	double CGaussian_Quadrature_Integrator::Transform(double y_diff_half, double y_sum_half, double t) const {
		return mFunction.Calculate_Transfer_Input(y_sum_half + y_diff_half * t) * y_diff_half;
	}

	double CGaussian_Quadrature_Integrator::Integrate(double time_start, double time_end) const {

		const double y_diff = time_end - time_start;
		const double y_sum = time_end + time_start;

		const double y_diff_half = y_diff * 0.5;
		const double y_sum_half = y_sum * 0.5;

		const double x0 = 0.555555556 * Transform(y_diff_half, y_sum_half, 0.7745966692);
		const double x1 = 0.888888889 * Transform(y_diff_half, y_sum_half, 0.0);
		const double x2 = 0.555555556 * Transform(y_diff_half, y_sum_half, -0.7745966692);

		return (x0 + x1 + x2);
	}
}
