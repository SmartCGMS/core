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

#include <list>
#include <variant>
#include <memory>

#include "../../../../common/utils/DebugHelper.h"

namespace gege
{
	constexpr double Value_Multiplier = 30.0; // this results to scaling on the interval of -15 to 15

	enum class NOperator
	{
		Greater_Than,
		Less_Than,

		count
	};

	enum class NQuantity
	{
		IG,
		IG_Slope,
		IG_Average_Last_0_5,
		IG_Average_Last_6_10,
		

		count
	};

	enum class NAction
	{
		Set_IBR,
		Set_Bolus,

		count
	};

	class CStart_Node;

	class CContext
	{
		private:
			// state variables
			double mState[static_cast<size_t>(NQuantity::count)];

			// output variables
			double mIBR = 0;
			double mBolus = 0;

			// start node (to run the evaluation)
			std::unique_ptr<CStart_Node> mStart_Node;

		public:
			CContext() : mState{ 0 }, mIBR{ 0 } {
				for (size_t i = 0; i < static_cast<size_t>(NQuantity::count); i++)
					mState[i] = 0;

				mStart_Node = std::make_unique<CStart_Node>();
			};

			void Set_State_Variable(const NQuantity q, const double value) { mState[static_cast<size_t>(q)] = value; }
			double Get_State_Variable(const NQuantity q) const { return mState[static_cast<size_t>(q)]; }

			double Get_Output_IBR() const { return mIBR; };
			double Get_Output_Bolus() const { return mBolus; };
			void Set_Output_IBR(const double ibr) { mIBR = ibr; };
			void Set_Output_Bolus(const double bolus) { mBolus = bolus; };

			void Evaluate();

			bool Parse(const std::vector<double>& genome);

			void Transcribe(std::vector<std::string>& target);
	};

	class CNode
	{
		public:
			virtual void Evaluate(CContext& ctx) = 0;
			virtual bool Parse(const std::vector<double>& genome, size_t& cur_idx) = 0;
			virtual void Transcribe(std::vector<std::string>& target) = 0;
	};

	class CBool_Expression_Node : public CNode
	{
		private:
			bool mEvaluation_Result = false;

			NQuantity mLHS;
			std::variant<NQuantity, double> mRHS;
			bool mRHS_Is_Double = false;

			NOperator mOperator;

		public:
			virtual void Evaluate(CContext& ctx) override
			{
				double lhs = ctx.Get_State_Variable(mLHS);
				double rhs = 0;

				if (mRHS_Is_Double)
					rhs = std::get<double>(mRHS);
				else
				{
					auto rhstype = std::get<NQuantity>(mRHS);
					rhs = ctx.Get_State_Variable(rhstype);
				}

				switch (mOperator)
				{
					case NOperator::Greater_Than:
						mEvaluation_Result = lhs > rhs;
						break;
					case NOperator::Less_Than:
						mEvaluation_Result = lhs < rhs;
						break;
				}
			}

			virtual bool Parse(const std::vector<double>& genome, size_t& cur_idx) override
			{
				// <bool_expression> ::= <quantity> <operator> <quantity> | <quantity> <operator> <constant>

				if (cur_idx + 3 >= genome.size())
					return false;

				mLHS = static_cast<NQuantity>(genome[cur_idx + 0] * (static_cast<double>(NQuantity::count) - 0.0001)); // scale to 0 - N.999 (trimming to integer gives us N buckets)
				mOperator = static_cast<NOperator>(genome[cur_idx + 1] * (static_cast<double>(NOperator::count) - 0.0001));

				if (genome[cur_idx] < 0.5) // rule 1
				{
					mRHS = static_cast<NQuantity>(genome[cur_idx + 2] * (static_cast<double>(NQuantity::count) - 0.0001));
					mRHS_Is_Double = false;
				}
				else // rule 2
				{
					mRHS = genome[cur_idx + 2] * Value_Multiplier - Value_Multiplier*0.5;
					mRHS_Is_Double = true;
				}

				cur_idx += 3;

				return true;
			}

			virtual void Transcribe(std::vector<std::string>& target) override
			{
				auto quant_to_str = [](NQuantity q) -> std::string {
					switch (q)
					{
						case NQuantity::IG: return "IG";
						case NQuantity::IG_Slope: return "IG_Slope";
						case NQuantity::IG_Average_Last_0_5: return "IG_Avg_0_5";
						case NQuantity::IG_Average_Last_6_10: return "IG_Avg_6_10";
					}

					return "???";
				};

				std::string condStr;

				condStr += quant_to_str(mLHS);
				condStr += " ";
				switch (mOperator)
				{
					case NOperator::Greater_Than: condStr += ">"; break;
					case NOperator::Less_Than: condStr += "<"; break;
					default: condStr += "?"; break;
				}
				condStr += " ";

				if (mRHS_Is_Double)
					condStr += std::to_string(std::get<double>(mRHS));
				else
					condStr += quant_to_str(std::get<NQuantity>(mRHS));

				target.push_back(condStr);
			}

			bool Get_Result() const
			{
				return mEvaluation_Result;
			}
	};

	class CCondition_Node : public CNode
	{
		private:
			std::list<CBool_Expression_Node> mExpressions;

		public:
			virtual void Evaluate(CContext& ctx) override
			{
				for (auto& exp : mExpressions)
					exp.Evaluate(ctx);
			}

			virtual bool Parse(const std::vector<double>& genome, size_t& cur_idx) override
			{
				// <cond> ::= <bool_expression> AND <bool_expression> | <bool_expression>

				if (cur_idx >= genome.size())
					return false;

				if (genome[cur_idx < 0.5]) // rule 1
				{
					mExpressions.push_back(CBool_Expression_Node());
					mExpressions.push_back(CBool_Expression_Node());
				}
				else // rule 2
				{
					mExpressions.push_back(CBool_Expression_Node());
				}

				cur_idx++;

				for (auto& exp : mExpressions)
				{
					if (!exp.Parse(genome, cur_idx))
						return false;
				}

				return true;
			}

			virtual void Transcribe(std::vector<std::string>& target) override
			{
				target.push_back("IF (");

				for (auto& exp : mExpressions)
					exp.Transcribe(target);

				target.push_back("   )");
			}

			bool Get_Result() const
			{
				// "AND" all expressions
				for (auto& exp : mExpressions)
				{
					if (!exp.Get_Result())
						return false;
				}

				return true;
			}
	};

	class CAction_Node : public CNode
	{
		private:
			NAction mAction;
			double mValue;

		public:
			virtual void Evaluate(CContext& ctx) override
			{
				switch (mAction)
				{
					case NAction::Set_IBR:
						ctx.Set_Output_IBR(std::max(mValue, 0.0));
						break;
					case NAction::Set_Bolus:
						ctx.Set_Output_Bolus(std::max(mValue, 0.0));
						break;
				}
			}

			virtual bool Parse(const std::vector<double>& genome, size_t& cur_idx) override
			{
				// <action> ::= SET_IBR(<value>)

				// we need an additional 1 parameter (1 encodes the rule, 1 encodes the value
				if (cur_idx + 1 >= genome.size())
					return false;

				mAction = static_cast<NAction>(genome[cur_idx] * (static_cast<double>(NAction::count) - 0.0001)); // scale to 0 - N.999 (trimming to integer gives us N buckets)
				mValue = genome[cur_idx + 1] * Value_Multiplier - Value_Multiplier * 0.5; // let us try to multiply the value by a constant Value_Multiplier for now, TODO: revise this

				cur_idx += 2;

				return true;
			}

			virtual void Transcribe(std::vector<std::string>& target) override
			{
				std::string actionStr;

				switch (mAction)
				{
					case NAction::Set_IBR:
						actionStr = "SET_IBR(";
						break;
					case NAction::Set_Bolus:
						actionStr = "SET_BOLUS(";
						break;
				}

				actionStr += std::to_string(mValue) + ")";

				target.push_back(actionStr);
			}
	};

	class CRule_Node : public CNode
	{
		private:
			bool mHas_Cond;
			CCondition_Node mCond;

			CAction_Node mAction;

		public:
			virtual void Evaluate(CContext& ctx) override
			{
				if (mHas_Cond)
					mCond.Evaluate(ctx);

				if (!mHas_Cond || mCond.Get_Result())
					mAction.Evaluate(ctx);
			}

			virtual bool Parse(const std::vector<double>& genome, size_t& cur_idx) override
			{
				// <rule> ::= <cond> <action> | <action>

				if (cur_idx >= genome.size())
					return false;

				//if (genome[cur_idx] < 0.5) // rule 1
				{
					cur_idx++;
					mHas_Cond = true;
					return mCond.Parse(genome, cur_idx) && mAction.Parse(genome, cur_idx);
				}
				/*else // rule 2
				{
					cur_idx++;
					mHas_Cond = false;
					return mAction.Parse(genome, cur_idx);
				}*/
			}

			virtual void Transcribe(std::vector<std::string>& target) override
			{
				if (mHas_Cond)
					mCond.Transcribe(target);

				mAction.Transcribe(target);
			}
	};

	class CStart_Node : public CNode
	{
		private:
			std::list<CRule_Node> mRules;

		public:
			virtual void Evaluate(CContext& ctx) override
			{
				for (auto& rule : mRules)
					rule.Evaluate(ctx);
			}

			virtual bool Parse(const std::vector<double>& genome, size_t& cur_idx) override
			{
				// <S> ::= <rule> | <rule> <S>

				do // rule 1
				{
					mRules.push_back(CRule_Node());
					cur_idx++;

					if (cur_idx >= genome.size())
						return false;

				} while (genome[cur_idx - 1] > 0.5); // rule 2

				for (auto& rule : mRules)
				{
					if (!rule.Parse(genome, cur_idx))
						return false;
				}

				return true;
			}

			virtual void Transcribe(std::vector<std::string>& target) override
			{
				for (auto& rule : mRules)
					rule.Transcribe(target);
			}
	};
}