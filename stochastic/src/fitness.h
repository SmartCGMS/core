#pragma once

#include "../..\..\common\rtl\SubjectLib.h"
#include "../..\..\common\rtl\SolverLib.h"

#include <vector>

#undef max



//#define D_PRINT_PARAMS

#ifdef D_PRINT_PARAMS

#include "../../../common/DebugHelper.h"
#include <atomic>

namespace stochastic_fitness {

	static std::atomic<int> params_print_counter{ 0 };

	template <typename TSolution>
	void Print_Params(const TSolution &solution) {
		TSolvedParams params = solution.As_Single_Params(0);
		switch (params.SolvingMethod) {
			case smDiffusion2:
				dprintf("\niteration: %d, p: %f, cg: %f; c: %f; dt: %f", params_print_counter.fetch_add(1), params.diffusion2.p, params.diffusion2.cg, params.diffusion2.c, params.diffusion2.dt);
				break;

			case smDiffusion_Ist_Prediction:
				dprintf("\niteration: %d, p: %f, cg: %f; c: %f; dt: %f", params_print_counter.fetch_add(1), params.diffusion_ist_prediction.retrospective.p, params.diffusion_ist_prediction.retrospective.cg, params.diffusion_ist_prediction.retrospective.c, params.diffusion_ist_prediction.retrospective.dt);
				break;

			default: break;

		}
	}
}
#endif



typedef struct {
	STimeSegment segment;
	SCalculation calculation;
} TSegmentInfo;

template <typename TSolution>
class CFitness {
protected:
	std::vector<TSegmentInfo> mSegments;
	size_t mCalculation_Id;
	bool mTest_Measured;	//whether the metric should be calculated with measured values or approximated
	size_t mLevels_Required;
public:
	CFitness(ITimeSegment **segments, const size_t segment_count, const size_t calculation_id) {
		for (size_t i = 0; i < segment_count; i++) {
			TSegmentInfo tmp;
			tmp.segment = make_shared_reference_ext<STimeSegment, ITimeSegment>(segments[i], true);
			tmp.calculation = tmp.segment.GetCalculation(calculation_id);

			if (Shared_Valid_All(tmp.segment, tmp.calculation)) mSegments.push_back(tmp);
		}	
	}
	
	void Set_Test_Measured(const bool test_measured, const size_t levels_required, floattype stepping) {
		unused(stepping);
		mTest_Measured = test_measured;
		mLevels_Required = levels_required;
	}

	floattype Calculate_Fitness(const TSolution &solution, SMetricCalculator &metric) const {

#ifdef D_PRINT_PARAMS
		stochastic_fitness::Print_Params(solution);
#endif

		//caller has to supply the metric to allow parallelization without inccuring an overhead
		//of creating new metric calculator instead of its mere resetting	
		metric->Reset();

		IMetricCalculator *measured_metric = nullptr;
		IMetricCalculator *approximated_metric = nullptr;
		if (mTest_Measured) measured_metric = metric.get();
		else approximated_metric = metric.get();

		floattype total_complexity = 0.0;

		for (const TSegmentInfo &segment : mSegments) {
			floattype local_complexity;
			if (segment.calculation->CalculateMetric(solution.As_Params(), approximated_metric, measured_metric, &local_complexity) != S_OK) 
					return std::numeric_limits<floattype>::max();	//we need to find a solution to all the segments
			total_complexity += local_complexity;
		}

		floattype result;
		size_t tmp_levels_accumulated;
		if (metric->Calculate(&result, &tmp_levels_accumulated, mLevels_Required) != S_OK)
			result = std::numeric_limits<floattype>::max();

		return result*total_complexity;
	}

	SCalculation original_calculation(size_t idx) {
		return mSegments[idx].calculation;
	}
};