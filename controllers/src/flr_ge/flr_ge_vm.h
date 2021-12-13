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

#include "../../../../common/utils/DebugHelper.h"

namespace flr_ge
{
	constexpr size_t value_ref_count = 20;

	enum class NInstruction
	{
		NOP = 0,
		CSIBR_1 = 1,
		CSBOL_1 = 2,
		CSIBR_2 = 3,
		CSBOL_2 = 4,

		count
	};

	static inline const char* Instruction_To_String(const NInstruction ins)
	{
		switch (ins)
		{
			case NInstruction::NOP: return "NOP";
			case NInstruction::CSIBR_1: return "CSIBR";
			case NInstruction::CSBOL_1: return "CSBOL";
			case NInstruction::CSIBR_2: return "CSIBR";
			case NInstruction::CSBOL_2: return "CSBOL";
		}
		return "???";
	}

	enum class NOperator
	{
		Less_Than,
		Greater_Than,

		count
	};

	static inline const char* Operator_To_String(const NOperator op)
	{
		switch (op)
		{
			case NOperator::Less_Than: return "<";
			case NOperator::Greater_Than: return ">";
		}
		return "?";
	}

	enum class NQuantity
	{
		IG,
		IG_Avg5,
		IG_Avg5_Slope,
		Minutes_From_Last_Bolus,

		count
	};

	static inline const char* Quantity_To_String(const NQuantity q)
	{
		switch (q)
		{
			case NQuantity::IG: return "IG";
			case NQuantity::IG_Avg5: return "IG_Avg5";
			case NQuantity::IG_Avg5_Slope: return "IG_Avg5_Slope";
			case NQuantity::Minutes_From_Last_Bolus: return "Minutes_From_Last_Bolus";
		}
		return "?";
	}

	enum class NOutput_Variable
	{
		IBR,
		Bolus,

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

	static inline  size_t classify_and_rescale(double& input, size_t bucket_count)
	{
		size_t out = classify(input, bucket_count);
		input = rescale(input, bucket_count);
		return out;
	}

	class CContext;

	class CRule
	{
		public:
			virtual bool Parse(const std::vector<double>& input, size_t cursor) = 0;
			virtual void Evaluate(CContext& ctx) = 0;
			virtual void Transcribe(CContext& ctx, std::vector<std::string>& target) const = 0;
	};

	class CContext
	{
		private:
			std::array<double, static_cast<size_t>(NQuantity::count)> mQuantities;
			std::array<double, static_cast<size_t>(NOutput_Variable::count)> mOutput;
			std::array<double, value_ref_count> mConstants;

			std::vector<std::unique_ptr<CRule>> mRules;

		public:
			CContext()
			{
				std::fill(mQuantities.begin(), mQuantities.end(), 0);
				std::fill(mOutput.begin(), mOutput.end(), 0);
				std::fill(mConstants.begin(), mConstants.end(), 0);
			}

			void Set_Constants(std::vector<double>& input, size_t start, size_t count)
			{
				for (size_t i = start; i < start + count; i++)
					mConstants[i - start] = input[i];
			}
			void Set_Quantity(const NQuantity q, const double value)
			{
				mQuantities[static_cast<size_t>(q)] = value;
			}

			void Set_Output(const NOutput_Variable ov, const double value)
			{
				mOutput[static_cast<size_t>(ov)] = std::max(0.0, value);
			}
			double Get_Quantity(const NQuantity q) const
			{
				return mQuantities[static_cast<size_t>(q)];
			}
			double Get_Constant(const size_t idx) const
			{
				return mConstants[idx];
			}
			double Get_Output(const NOutput_Variable ov) const
			{
				return mOutput[static_cast<size_t>(ov)];
			}

			bool Parse(const std::vector<double>& input, size_t rule_count, size_t rule_length);

			void Evaluate()
			{
				for (auto& rule : mRules)
				{
					if (rule)
						rule->Evaluate(*this);
				}
			}

			void Transcribe(std::vector<std::string>& target)
			{
				for (auto& rule : mRules)
				{
					if (rule)
						rule->Transcribe(*this, target);
				}
			}
	};

	class CCond_Rule : public CRule
	{
		protected:
			struct TConditional
			{
				NOperator op;
				NQuantity lhs;

				bool is_rhs_quantity;
				union
				{
					NQuantity q;
					size_t v;
				} rhs; // yeah, we have std::variant, but let us speed things up
			};

		public:
			bool Decode_Conditional(const double input, TConditional& target)
			{
				if (input < 0 || input > 1)
					return false;

				double in = input;

				target.op = classify_and_rescale<NOperator>(in);
				target.lhs = classify_and_rescale<NQuantity>(in);
				auto rhs_raw = classify(in, static_cast<size_t>(NQuantity::count) + value_ref_count);

				if (rhs_raw < static_cast<size_t>(NQuantity::count)) // 0 .. N
				{
					target.is_rhs_quantity = true;
					target.rhs.q = static_cast<NQuantity>(rhs_raw);
				}
				else // 0 - (value_ref_count-1)
				{
					target.is_rhs_quantity = false;
					target.rhs.v = rhs_raw - static_cast<size_t>(NQuantity::count);
				}

				return true;
			}

			bool Evaluate_Conditional(CContext& ctx, TConditional& target)
			{
				double lhs = ctx.Get_Quantity(target.lhs);
				double rhs = target.is_rhs_quantity ? ctx.Get_Quantity(target.rhs.q) : ctx.Get_Constant(target.rhs.v);

				switch (target.op)
				{
					case NOperator::Less_Than:
						return lhs < rhs;
					case NOperator::Greater_Than:
						return lhs > rhs;
				}

				return false;
			}

			void Transcribe_Conditional(const TConditional& cond, CContext& ctx, std::string& target) const
			{
				target += "(";
				target += Quantity_To_String(cond.lhs);
				target += Operator_To_String(cond.op);
				if (cond.is_rhs_quantity)
					target += Quantity_To_String(cond.rhs.q);
				else
					target += std::to_string(ctx.Get_Constant(cond.rhs.v));
				target += ")";
			}
	};

	template<NInstruction Ins, NOutput_Variable OVar>
	class CCond_Set_1 : public CCond_Rule
	{
		private:
			TConditional mCond_1;
			size_t mValue_Ref;

		public:
			virtual bool Parse(const std::vector<double>& input, size_t cursor) override
			{
				// CSxxx_1 <condition> <NULL> <value_ref>

				mValue_Ref = classify(input[cursor + 3], value_ref_count);
				return Decode_Conditional(input[cursor + 1], mCond_1);
			}

			virtual void Evaluate(CContext& ctx) override
			{
				if (Evaluate_Conditional(ctx, mCond_1))
					ctx.Set_Output(OVar, ctx.Get_Constant(mValue_Ref));
			}

			virtual void Transcribe(CContext& ctx, std::vector<std::string>& target) const override
			{
				std::string trans = Instruction_To_String(Ins);
				trans += " ";
				Transcribe_Conditional(mCond_1, ctx, trans);
				trans += " ";
				trans += std::to_string(ctx.Get_Constant(mValue_Ref));

				target.push_back(trans);
			}
	};

	template<NInstruction Ins, NOutput_Variable OVar>
	class CCond_Set_2 : public CCond_Rule
	{
		private:
			TConditional mCond_1, mCond_2;
			size_t mValue_Ref;

		public:
			virtual bool Parse(const std::vector<double>& input, size_t cursor) override
			{
				// CSxxx_2 <condition> <condition> <value_ref>

				mValue_Ref = classify(input[cursor + 3], value_ref_count);
				return Decode_Conditional(input[cursor + 1], mCond_1) && Decode_Conditional(input[cursor + 2], mCond_2);
			}

			virtual void Evaluate(CContext& ctx) override
			{
				if (Evaluate_Conditional(ctx, mCond_1) && Evaluate_Conditional(ctx, mCond_2))
					ctx.Set_Output(OVar, ctx.Get_Constant(mValue_Ref));
			}

			virtual void Transcribe(CContext& ctx, std::vector<std::string>& target) const override
			{
				std::string trans = Instruction_To_String(Ins);
				trans += " ";
				Transcribe_Conditional(mCond_1, ctx, trans);
				trans += " ";
				Transcribe_Conditional(mCond_2, ctx, trans);
				trans += " ";
				trans += std::to_string(ctx.Get_Constant(mValue_Ref));

				target.push_back(trans);
			}
	};
}
