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

#pragma once

//#include "gct3_transfer_functions.h"

namespace gct3_model {

	class CTransfer_Function;

	class IIntegrator {
		protected:
			const CTransfer_Function& mFunction;

		public:
			IIntegrator(const CTransfer_Function& fnc) : mFunction(fnc) {
				//
			}
			virtual ~IIntegrator() = default;

			virtual double Integrate(double time_start, double time_end) const = 0;
	};

	// Rectangular rule for integration
	class CRectangular_Integrator : public IIntegrator {
		public:
			using IIntegrator::IIntegrator;

			double Integrate(double time_start, double time_end) const override;
	};

	// Single midpoint rule for integration
	class CMidpoint_Integrator : public IIntegrator {
		public:
			using IIntegrator::IIntegrator;

			double Integrate(double time_start, double time_end) const override;
	};

	// Simspon's 1/3 rule for integration
	class CSimpson_1_3_Rule_Integrator : public IIntegrator {
		public:
			using IIntegrator::IIntegrator;

			double Integrate(double time_start, double time_end) const override;
	};

	// Gaussian quadrature rule for integration
	class CGaussian_Quadrature_Integrator : public IIntegrator {
		private:
			// transform to <-1;1> interval
			double Transform(double y_diff_half, double y_sum_half, double t) const;

		public:
			using IIntegrator::IIntegrator;

			double Integrate(double time_start, double time_end) const override;
	};
}
