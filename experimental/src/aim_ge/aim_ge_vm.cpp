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

#include "aim_ge_vm.h"

namespace aim_ge
{
	bool COperator_Rule::Parse(double input, size_t recursion_level)
	{
		// <op> ::= + | - | *

		// it has been parsed for us
		if (!mLHS_Rule || !mRHS_Rule)
		{

			auto switch_and_apply = [](NInstruction type) -> std::unique_ptr<CRule> {
				switch (type)
				{
					case NInstruction::E_GLUC:
					case NInstruction::E_INS:
					case NInstruction::E_CHO:
						return std::make_unique<CQuantity_Term_Rule>(type);
						break;
					//case NInstruction::FUNC:
						//return std::make_unique<CFunction_Rule>();
						//break;
					default: return nullptr;
				}

				return nullptr;
			};

			mLHS_Rule = switch_and_apply(mLHS);
			mLHS_Rule->Parse(input, recursion_level + 1);
			mRHS_Rule = switch_and_apply(mRHS);
			mRHS_Rule->Parse(input, recursion_level + 1);
		}

		if (mOperator == NOperator::count)
			mOperator = classify_and_rescale<NOperator>(input);

		return true;
	}

	bool CAlgebraic_Quotient_Rule::Parse(double input, size_t recursion_level)
	{
		// <op> ::= + | - | *

		// it has been parsed for us
		if (!mLHS_Rule || !mRHS_Rule)
		{
			auto switch_and_apply = [](NInstruction type) -> std::unique_ptr<CRule> {
				switch (type)
				{
					case NInstruction::E_GLUC:
					case NInstruction::E_INS:
					case NInstruction::E_CHO:
						return std::make_unique<CQuantity_Term_Rule>(type);
						break;
						//case NInstruction::FUNC:
							//return std::make_unique<CFunction_Rule>();
							//break;
					default: return nullptr;
				}

				return nullptr;
			};

			mLHS_Rule = switch_and_apply(mLHS);
			mLHS_Rule->Parse(input, recursion_level + 1);
			mRHS_Rule = switch_and_apply(mRHS);
			mRHS_Rule->Parse(input, recursion_level + 1);
		}

		return true;
	}

	bool CFunction_Rule::Parse(double input, size_t recusion_level)
	{
		if (!mOperand_Rule)
		{
			mOperand_Rule = std::make_unique<CQuantity_Term_Rule>(mOperand);
			mOperand_Rule->Parse(input, recusion_level + 1);
		}

		mFunction = classify_and_rescale<NFunction>(input);

		return true;
	}
}
