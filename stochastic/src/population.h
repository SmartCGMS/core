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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include <string>
#include <cmath>
#include <fstream>
#include <iostream>

namespace population_helpers {
	// class for dumping population in order to be processed later separately
	class CPopulation_Dump {
		private:
			// file for output
			std::ofstream mFile;
			// problem size (parameters count)
			const size_t mProblem_Size;

		public:
			CPopulation_Dump(const size_t problemSize)
				: mProblem_Size(problemSize) {
				//
			}

			// initializes file output; separated from constructor to allow instantiating without actually doing any disk I/O when not in some kind of debug mode
			void Init(const std::string& fileName = "dump.csv") {
				mFile.open(fileName);

				// CSV header
				mFile << "generation;member_ordinal;fitness";
				for (size_t i = 0; i < mProblem_Size; i++) {
					mFile << ";param_" << i;
				}
				mFile << std::endl;
			}

			virtual ~CPopulation_Dump() = default;

			// dump population in a given vector
			// TODO: overload Dump method to allow continuous vector of parameters conserving the whole population (batch objective calculation-ready algorithms)
			template<typename TPopulation_Vector>
			void Dump(const size_t generation, const TPopulation_Vector& population, std::function<double(const typename TPopulation_Vector::value_type&)> extractFitness, std::function<double(const typename TPopulation_Vector::value_type&, size_t paramIdx)> extractParameter) {
				size_t memberOrdinal = 0;

				for (const auto& member : population) {
					mFile << generation << ";" << memberOrdinal << ";" << extractFitness(member);

					for (size_t pidx = 0; pidx < mProblem_Size; pidx++) {
						mFile << ";" << extractParameter(member, pidx);
					}

					memberOrdinal++;

					mFile << std::endl;
				}
			}
	};
}
