#pragma once

#include "../..\..\common\rtl\SolverLib.h"
#include "../..\..\common\rtl\referencedImpl.h"

#include <vector>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

template <typename TSolver, typename TSolution, typename TFitness>
class CSolver_Skeleton : public ISolver, public virtual CReferenced {
protected:
	TFitness mFitness;
	SMetricFactory mMetric_Factory;
protected:
	const size_t mSegmentCount;
	const size_t mSuggested_Max_Count = 20;
	std::vector<TSolution> mInitial_Solutions;
public:
	CSolver_Skeleton(ITimeSegment **segments, size_t segmentcount, IMetricFactory *metricfactory, size_t calculation_id) :	
		mMetric_Factory(make_shared_reference_ext<SMetricFactory, IMetricFactory>(metricfactory, true)),
		mFitness(segments, segmentcount, calculation_id),
		mSegmentCount(segmentcount) { 

		//Try to load any previously stored parameters
		size_t suggested_count = 0;
		std::vector<TSolvedParams> suggestions(mSuggested_Max_Count);		
		suggestions[0].SolvingMethod = Calculation_Id_to_Base_Solving_Method(calculation_id);
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
	virtual ~CSolver_Skeleton() {};

	virtual HRESULT IfaceCalling Solve(TSolvingParams solving, TSolvedParams *solved, TSolverProgress *progress) {		
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

#pragma warning( pop )