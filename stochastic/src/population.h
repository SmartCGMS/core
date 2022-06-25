#pragma once

#include <string>
#include <cmath>
#include <fstream>
#include <iostream>

namespace population_helpers
{
	// class for dumping population in order to be processed later separately
	class CPopulation_Dump
	{
		private:
			// file for output
			std::ofstream mFile;
			// problem size (parameters count)
			const size_t mProblem_Size;

		public:
			CPopulation_Dump(const size_t problemSize)
				: mProblem_Size(problemSize)
			{
				//
			}

			// initializes file output; separated from constructor to allow instantiating without actually doing any disk I/O when not in some kind of debug mode
			void Init(const std::string& fileName = "dump.csv")
			{
				mFile.open(fileName);

				// CSV header
				mFile << "generation;member_ordinal;fitness";
				for (size_t i = 0; i < mProblem_Size; i++)
					mFile << ";param_" << i;
				mFile << std::endl;
			}

			virtual ~CPopulation_Dump() = default;

			// dump population in a given vector
			// TODO: overload Dump method to allow continuous vector of parameters conserving the whole population (batch objective calculation-ready algorithms)
			template<typename TPopulation_Vector>
			void Dump(const size_t generation, const TPopulation_Vector& population, std::function<double(const typename TPopulation_Vector::value_type&)> extractFitness, std::function<double(const typename TPopulation_Vector::value_type&, size_t paramIdx)> extractParameter)
			{
				size_t memberOrdinal = 0;

				for (const auto& member : population)
				{
					mFile << generation << ";" << memberOrdinal << ";" << extractFitness(member);

					for (size_t pidx = 0; pidx < mProblem_Size; pidx++)
						mFile << ";" << extractParameter(member, pidx);

					memberOrdinal++;

					mFile << std::endl;
				}
			}
	};
}
