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

/*

ISA:

	<condition> is encoded using arithmetic encoding
				- first subdivision selects the operator (<operator>)
				- second subdivision selects the LHS (<quantity>)
				- third subdivision selects the RHS (<quantity> or <value_ref>)
					- the interval contains both quantities and value_refs (unlike LHS operand)

		e.g.: 0.99
		<--------LT--------|--------GT-------->
		                  /                   |
		          --------                    |
		         /                            |
		 --------                             |
		|                                     |
		<----IG---|-IG_slp-|-IG_avg-|-IG_avg2->
		                            /         |
		          ------------------          |
		         /                            |
		 --------                             |
		|                                     |
		<-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|->
		(quants.|         value_refs          )
		                                     ^
		--> this gets decoded as IG_avg2 > value_10 (given the last value is indexed with 10)

	<value_ref> is a reference to 'constants' region (interval subdivision)

	ISA:

	NOP     <NULL>      <NULL>      <NULL>					- no operation, decoder skips this
	CSIBR_1 <condition> <NULL>      <value_ref>				- conditionally sets the IBR to a given value (referenced by value_ref)
	CSBOL_1 <condition> <NULL>      <value_ref>				- conditionally sets the bolus to a given value (referenced by value_ref)
	CSIBR_2 <condition> <condition> <value_ref>				- same as CSIBR_1, but with two conditions joined with AND operator
	CSBOL_2 <condition> <condition> <value_ref>				- same as CSBOL_1, but with two conditions joined with AND operator

	DISCARDED:

	SIBR  <value_ref>   <NULL>      <NULL>					- sets IBR to given value (referenced by value_ref)

*/

#include <memory>
#include <vector>
#include <array>
#include <cmath>
#include <cassert>
#include <functional>

#include "../../../../common/utils/DebugHelper.h"
#include "../../../../common/iface/DeviceIface.h"

using namespace scgms::literals;

#undef min
#undef max

namespace aim_ge
{
	constexpr double Prediction_Horizon = 120.0_min;

	constexpr size_t Glucose_Hist_Mins = 120;
	constexpr size_t Insulin_Future_Mins = 120;
	constexpr size_t CHO_Future_Mins = 120;

	constexpr size_t Step_Size_Mins = 5;

	constexpr size_t Max_Rule_Recursion = 6;

	enum class NInstruction
	{
		NOP = 0,

		OP,
		AQ,
		FUNC,

		GLUC,
		INS,
		CHO,

		E_GLUC,
		E_INS,
		E_CHO,

		NUMBER,

		count
	};

	enum class NFunction
	{
		PSQRT = 0,
		PLOG,
		SIN,
		TANH,
		EXP,

		count
	};

	enum class NOperator
	{
		ADD,
		SUB,
		MUL,

		count
	};

	enum class NInput_Quantity
	{
		GLUC,
		INS,
		CHO,

		count
	};

	static inline const char* Instruction_To_String(const NInstruction ins)
	{
		switch (ins)
		{
			case NInstruction::NOP: return "NOP";
			case NInstruction::OP: return "OP";
			//case NInstruction::AQ: return "AQ";
			case NInstruction::FUNC: return "FUNC";
			case NInstruction::GLUC: return "GLUC";
			case NInstruction::INS: return "INS";
			case NInstruction::CHO: return "CHO";
			case NInstruction::E_GLUC: return "E_GLUC";
			case NInstruction::E_INS: return "E_INS";
			case NInstruction::E_CHO: return "E_CHO";
			case NInstruction::NUMBER: return "NUMBER";
			default: return "???";
		}
		return "???";
	}

	static inline NInput_Quantity Instruction_To_Input_Quantity(NInstruction ins)
	{
		switch (ins)
		{
			case NInstruction::E_GLUC: return NInput_Quantity::GLUC;
			case NInstruction::E_INS: return NInput_Quantity::INS;
			case NInstruction::E_CHO: return NInput_Quantity::CHO;
			default: return NInput_Quantity::count;
		}

		return NInput_Quantity::count;
	}

	static inline const char* Function_To_String(const NFunction fnc)
	{
		switch (fnc)
		{
			case NFunction::PSQRT: return "psqrt";
			case NFunction::PLOG: return "plog";
			case NFunction::SIN: return "sin";
			case NFunction::TANH: return "tanh";
			case NFunction::EXP: return "exp";
			default: return "?";
		}
		return "?";
	}

	static inline const char* Operator_To_String(const NOperator op)
	{
		switch (op)
		{
			case NOperator::ADD: return "+";
			case NOperator::SUB: return "-";
			case NOperator::MUL: return "*";
			default: return "?";
		}
		return "?";
	}

	static inline const char* Input_Quantity_To_String(const NInput_Quantity q)
	{
		switch (q)
		{
			case NInput_Quantity::GLUC: return "G";
			case NInput_Quantity::INS: return "I";
			case NInput_Quantity::CHO: return "C";
			default: return "?";
		}
		return "?";
	}

	enum class NOutput_Variable
	{
		IG,

		count
	};

	template<typename T>
	static inline T classify(const double input)
	{
		assert(input >= 0.0 && input <= 1.0 && "Invalid input range");
		return static_cast<T>( input * (static_cast<double>(T::count) - 0.001) ); // scale to range 0 .. (N-1).999 and then cast to an integral type, so we obtain more-or-less equally-sized buckets
	}

	template<typename T>
	static inline double rescale(const double input)
	{
		return std::fmod(input, 1.0 / static_cast<double>(T::count)) * static_cast<double>(T::count);
	}

	template<typename T>
	static inline T classify_and_rescale(double& input)
	{
		T out = classify<T>(input);
		input = rescale<T>(input);
		return out;
	}

	static inline size_t classify(const double input, const size_t bucket_count)
	{
		assert(input >= 0.0 && input <= 1.0 && "Invalid input range");
		return static_cast<size_t>(input * (static_cast<double>(bucket_count) - 0.001));
	}

	static inline double rescale(const double input, const size_t bucket_count)
	{
		return std::fmod(input, 1.0 / static_cast<double>(bucket_count)) * static_cast<double>(bucket_count);
	}

	static inline size_t classify_and_rescale(double& input, size_t bucket_count)
	{
		size_t out = classify(input, bucket_count);
		input = rescale(input, bucket_count);
		return out;
	}

	class CContext;

	class CRule
	{
		public:
			virtual ~CRule() = default;
			virtual bool Parse(double input, size_t recusion_level) = 0;
			virtual double Evaluate(CContext& ctx) = 0;
			virtual void Transcribe(CContext& ctx, std::vector<std::string>& target) const = 0;
	};

	class IQuantity_Source
	{
		public:
			virtual double Get_Input_Quantity(NInput_Quantity q, size_t timeOffset) = 0;
	};

	class CContext
	{
		private:
			std::array<double, static_cast<size_t>(NOutput_Variable::count)> mOutput;

			std::unique_ptr<CRule> mRoot_Rule;

			IQuantity_Source& mQuantity_Source;

		public:
			CContext(IQuantity_Source& source) : mQuantity_Source(source)
			{
				std::fill(mOutput.begin(), mOutput.end(), 0);
			}

			void Set_Output(const NOutput_Variable ov, const double value)
			{
				mOutput[static_cast<size_t>(ov)] = std::max(0.0, value);
			}

			double Get_Output(const NOutput_Variable ov) const
			{
				return mOutput[static_cast<size_t>(ov)];
			}

			double Get_Input_Quantity(NInput_Quantity q, size_t timeOffset)
			{
				return mQuantity_Source.Get_Input_Quantity(q, timeOffset);
			}

			bool Parse(const std::vector<double>& input);

			double Evaluate()
			{
				double val = mRoot_Rule->Evaluate(*this);

				Set_Output(NOutput_Variable::IG, val);

				return val;
			}

			void Transcribe(std::vector<std::string>& target)
			{
				mRoot_Rule->Transcribe(*this, target);
			}

	};

	class COperator_Rule : public CRule
	{
		private:
			NOperator mOperator = NOperator::count;
			NInstruction mLHS, mRHS;

			std::unique_ptr<CRule> mLHS_Rule, mRHS_Rule;

		public:
			COperator_Rule(NInstruction lhs, NInstruction rhs)
				: mLHS(lhs), mRHS(rhs)
			{
				//
			}

			COperator_Rule(std::unique_ptr<CRule>&& lhs, std::unique_ptr<CRule>&& rhs, NOperator op = NOperator::count)
				: mOperator(op), mLHS_Rule(std::move(lhs)), mRHS_Rule(std::move(rhs))
			{
				//
			}

			virtual bool Parse(double input, size_t recusion_level) override;

			virtual double Evaluate(CContext& ctx) override
			{
				double lhs = mLHS_Rule->Evaluate(ctx);
				double rhs = mRHS_Rule->Evaluate(ctx);

				switch (mOperator)
				{
					case NOperator::ADD:
						return lhs + rhs;
					case NOperator::SUB:
						return lhs - rhs;
					case NOperator::MUL:
						return lhs * rhs;
					default: break;
				}

				return 0;
			}

			virtual void Transcribe(CContext& ctx, std::vector<std::string>& target) const override
			{
				switch (mOperator)
				{
					case NOperator::ADD:
					case NOperator::SUB:
					case NOperator::MUL:
						mLHS_Rule->Transcribe(ctx, target);
						target.push_back(Operator_To_String(mOperator));
						mRHS_Rule->Transcribe(ctx, target);
						break;
					default: break;
				}
			}
	};

	class CAlgebraic_Quotient_Rule : public CRule
	{
		private:
			NInstruction mLHS, mRHS;

			std::unique_ptr<CRule> mLHS_Rule, mRHS_Rule;

		public:
			CAlgebraic_Quotient_Rule(NInstruction lhs, NInstruction rhs)
				: mLHS(lhs), mRHS(rhs)
			{
				//
			}

			CAlgebraic_Quotient_Rule(std::unique_ptr<CRule>&& lhs, std::unique_ptr<CRule>&& rhs)
				: mLHS_Rule(std::move(lhs)), mRHS_Rule(std::move(rhs))
			{
				//
			}

			virtual bool Parse(double input, size_t recusion_level) override;

			virtual double Evaluate(CContext& ctx) override
			{
				double lhs = mLHS_Rule->Evaluate(ctx);
				double rhs = mRHS_Rule->Evaluate(ctx);

				return lhs / std::sqrt(1 + rhs * rhs);
			}

			virtual void Transcribe(CContext& ctx, std::vector<std::string>& target) const override
			{
				target.push_back("aq(");
				mLHS_Rule->Transcribe(ctx, target);
				target.push_back(",");
				mRHS_Rule->Transcribe(ctx, target);
				target.push_back(")");
			}
	};

	class CFunction_Rule : public CRule
	{
		private:
			NFunction mFunction = NFunction::count;
			NInstruction mOperand = NInstruction::count;
			std::unique_ptr<CRule> mOperand_Rule;

		public:
			CFunction_Rule(NInstruction operand)
				: mOperand(operand)
			{
				//
			}

			CFunction_Rule(std::unique_ptr<CRule>&& operand)
				: mOperand_Rule(std::move(operand))
			{
				//
			}

			virtual bool Parse(double input, size_t recusion_level) override;

			virtual double Evaluate(CContext& ctx) override
			{
				double operand_val = mOperand_Rule->Evaluate(ctx);

				switch (mFunction)
				{
					case NFunction::PSQRT:
						return std::sqrt(std::abs(operand_val));
					case NFunction::PLOG:
						return std::log(1 + std::abs(operand_val));
					case NFunction::EXP:
						return std::exp(operand_val);
					case NFunction::SIN:
						return std::sin(operand_val);
					case NFunction::TANH:
						return std::tanh(operand_val);
					default: return 0;
				}

				return 0;
			}

			virtual void Transcribe(CContext& ctx, std::vector<std::string>& target) const override
			{
				target.push_back(Function_To_String(mFunction));
				target.push_back("(");
				mOperand_Rule->Transcribe(ctx, target);
				target.push_back(")");
			}
	};

	class CInput_Quantity_Rule : public CRule
	{
		private:
			NInput_Quantity mQuantity;
			size_t mTime_Offset_Mins = 0;

		public:
			CInput_Quantity_Rule(NInput_Quantity q)
				: mQuantity(q)
			{
				//
			}

			virtual bool Parse(double input, size_t recusion_level) override
			{
				// <gluc> ::= G(t) | G(t-dt) | ... | G(t-k*dt)
				// <ins> ::= I(t) | I(t+dt) | ... | I(t+h*dt)
				// <cho> ::= C(t) | C(t+dt) | ... | C(t+h*dt)

				switch (mQuantity)
				{
					case NInput_Quantity::GLUC:
						mTime_Offset_Mins = classify_and_rescale(input, 1 + Glucose_Hist_Mins / Step_Size_Mins);
						break;
					case NInput_Quantity::INS:
						mTime_Offset_Mins = classify_and_rescale(input, 1 + Insulin_Future_Mins / Step_Size_Mins);
						break;
					case NInput_Quantity::CHO:
						mTime_Offset_Mins = classify_and_rescale(input, 1 + CHO_Future_Mins / Step_Size_Mins);
						break;
					default: break;
				}

				return true;
			}

			virtual double Evaluate(CContext& ctx) override
			{
				return ctx.Get_Input_Quantity(mQuantity, mTime_Offset_Mins);
			}

			virtual void Transcribe(CContext& ctx, std::vector<std::string>& target) const override
			{
				target.push_back(Input_Quantity_To_String(mQuantity));
				target.push_back("(t");

				switch (mQuantity)
				{
					case NInput_Quantity::GLUC:
						target.push_back("-" + std::to_string(mTime_Offset_Mins * Step_Size_Mins));
						break;
					case NInput_Quantity::INS:
					case NInput_Quantity::CHO:
						target.push_back("+" + std::to_string(mTime_Offset_Mins * Step_Size_Mins));
						break;
					default: break;
				}

				target.push_back(")");
			}
	};

	class CNumber_Rule : public CRule
	{
		private:
			double mValue = 0;

		public:
			virtual bool Parse(double input, size_t recusion_level) override
			{
				// <number> ::= <d>.<d> | <d>

				size_t rule = classify_and_rescale(input, 2);

				switch (rule)
				{
					case 0: // <d>.<d>
					{
						size_t dec_part = classify_and_rescale(input, 100);
						size_t frac_part = classify(input, 100);

						mValue = static_cast<double>(dec_part) + static_cast<double>(frac_part) / 100.0;
						break;
					}
					case 1: // <d>
						mValue = static_cast<double>(static_cast<int>(input * 100)); // 0:99
						break;
				}

				return true;
			}

			virtual double Evaluate(CContext& ctx) override
			{
				return mValue;
			}

			virtual void Transcribe(CContext& ctx, std::vector<std::string>& target) const override
			{
				target.push_back(std::to_string(mValue));
			}
	};

	class CQuantity_Term_Rule : public CRule
	{
		private:
			NInstruction mQuantity_Term;

			std::unique_ptr<CRule> mRule;

		public:
			CQuantity_Term_Rule(NInstruction termtype)
				: mQuantity_Term(termtype)
			{
				//
			}

			virtual bool Parse(double input, size_t recusion_level) override
			{
				// ORIG: <e_ABX> ::= <e_ABC> <op> <e_ABC> | aq(<e_ABC>, <e_ABC>) | <func>(<e_ABC>) | <ABC> | <number>

				size_t rule = classify_and_rescale(input, (recusion_level < Max_Rule_Recursion) ? 5 : 2); // disallow further recursion if we reached the limit

				switch (rule)
				{
					case 0: // <ABC>
						mRule = std::make_unique<CInput_Quantity_Rule>(Instruction_To_Input_Quantity(mQuantity_Term));
						break;
					case 1: // <number>
						mRule = std::make_unique<CNumber_Rule>();
						break;
					case 2: // aq(<e_ABC>, <e_ABC>)
						mRule = std::make_unique<CAlgebraic_Quotient_Rule>(mQuantity_Term, mQuantity_Term);
						break;
					case 3: // <e_ABC> <op> <e_ABC>
						mRule = std::make_unique<COperator_Rule>(mQuantity_Term, mQuantity_Term);
						break;
					case 4: // <func>(<e_ABC>)
						mRule = std::make_unique<CFunction_Rule>(mQuantity_Term);
						break;
				}

				mRule->Parse(input, recusion_level + 1);

				return true;
			}

			virtual double Evaluate(CContext& ctx) override
			{
				return mRule->Evaluate(ctx);
			}

			virtual void Transcribe(CContext& ctx, std::vector<std::string>& target) const override
			{
				mRule->Transcribe(ctx, target);
			}
	};
}
