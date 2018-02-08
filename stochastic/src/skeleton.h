#pragma once

#include "../..\..\common\rtl\SolverLib.h"
#include "../..\..\common\rtl\referencedImpl.h"

#include <vector>

template <typename TSolver, typename TSolution, typename TFitness>
class CSolver_Skeleton {
protected:
	TFitness mFitness;
	glucose::SFactory mMetric;
protected:
	const std::vector<glucose::STime_Segment> mSegments;
	const size_t mSuggested_Max_Count = 20;
	std::vector<TSolution> mInitial_Solutions;
public:
	CSolver_Skeleton(std::vector<glucose::STime_Segment> &segments, glucose::SMetric &metric, const GUID &signal_id) :
		mSegments{ segments }, mMetric{ metric }, mFitness{segments, signal_id } {


		//Try to load any previously stored parameters
		size_t suggested_count = 0;
		std::vector<glucose::IModel_Parameter_Vector> suggestions(mSuggested_Max_Count);		
		if (SUCCEEDED(segments[0]->SuggestParams(suggestions.data(), suggestions.size(), &suggested_count,
			calculation_id, metricfactory))) {
			//S_OK means loaded stored parameters
			//S_FALSE means default parameters - better than nothing

			//=>copy the suggested params
			mInitial_Solutions.reserve(suggested_count);
			for (size_t i = 0; i < suggested_count; i++) {
				TSolution suggestion = TSolution::From_Params(suggestions[i], mSegmentCount);
				mInitial_Solutions.push_back(suggestion);
			}
			
		}
	};	

	HRESULT Solve(TSolvingParams solving, TSolvedParams *solved, TSolverProgress *progress) {		
		try {
			TSolution lower_bound, upper_bound;
			TSolution::Convert_Bounds(solving, lower_bound, upper_bound, mSegmentCount);

			mFitness.Set_Test_Measured(solving.diffusion4.UseMeasuredBlood != 0, solving.MinimumLevelsRequired, solving.Stepping);

			TSolver solver(mInitial_Solutions, lower_bound, upper_bound, mFitness, mMetric_Factory);

			if (is_composite_solution<TSolution>::value) {
				auto solution = solver.Solve(*progress);
				for (size_t i = 0; i < mSegmentCount; i++) {
					TSolvedParams tmp = solution.As_Single_Params(i);
					if (i == 0) *solved = tmp;

					//and auto apply the computed paramters
					mFitness.original_calculation(i)->ReCalculate(solving.Stepping, tmp);
				}
			}
			else {
				*solved = solver.Solve(*progress).As_Single_Params(0);
			}
		}
		catch (...) {
			return E_FAIL;
		}
		
			
		return S_OK;
	}
	
};