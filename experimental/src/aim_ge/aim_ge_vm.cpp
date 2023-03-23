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
