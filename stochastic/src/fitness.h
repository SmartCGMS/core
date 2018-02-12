#pragma once

#include "../..\..\common\rtl\SolverLib.h"

#include <Eigen/Dense>
#include <tbb/concurrent_queue.h>
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
		//to be implemented
	}
}
#endif


struct TShared_Solver_Setup {
	const GUID *solver_id; const GUID *signal_id;
	const std::vector<glucose::STime_Segment> segments;
	const glucose::SMetric metric;	const size_t levels_required; const char use_measured_levels;
	const glucose::SModel_Parameter_Vector lower_bound, upper_bound;
	const std::vector<glucose::SModel_Parameter_Vector> solution_hints;
	glucose::SModel_Parameter_Vector solved_parameters;
	glucose::TSolver_Progress &progress;
};

using TVector1D = std::unique_ptr<Eigen::Matrix<double, 1, Eigen::Dynamic, Eigen::RowMajor>>;

struct TSegment_Info {
	glucose::STime_Segment segment;
	glucose::SSignal signal;
	std::vector<double> reference_time;
	TVector1D reference_level;
};

template <typename TSolution>
class CFitness {
protected:
	std::vector<TSegment_Info> mSegment_Info;	
	size_t mLevels_Required;
	size_t mMax_Levels_Per_Segment;	//to avoid multiple resize of memory block when calculating the error
	tbb::concurrent_queue<TVector1D> mTemporal_Levels;
public:
	CFitness(TShared_Solver_Setup &setup) : mLevels_Required (setup.levels_required) {

		mMax_Levels_Per_Segment = 0;

		for (auto &setup_segment : setup.segments) {
			TSegment_Info info;
			info.segment = setup_segment;
			info.signal = setup_segment.Get_Signal(setup.signal_id);			

			if (info.signal) {
				size_t levels_count;
				if (info.signal->Get_Discrete_Bounds(nullptr, &levels_count) == S_OK) {
					std::vector<glucose::TLevel> reference_levels (levels_count);
					
					std::vector<double> local_reference_level;	//to convert it later into the Eigen Matrix

					//prepare arrays with reference levels and their times
					if (info.signal->Get_Discrete_Levels(reference_levels.data(), reference_levels.size(), &levels_count) == S_SOK) {
						for (const auto &reference_level : reference_levels) {
							info.reference_time.push_back(reference_time.date_time);
							reference_level.push_back(reference_level.level);
						}
					}
					

					//if desired, replace them continous signal approdximation
					if (setup.use_measured_levels == 0) {
						info.signal->Get_Continuous_Levels(nullptr, info.reference_time.data(), local_reference_level.data(), info.reference_time.size(), &levels_count, glucose::apxNoDerivative);
							//we are not interested in checking the possibly error, because we will be left with meeasured levels at least
					}

					//do we have everyting we need to test this segment?
					if (!info.reference_time.empty()) {
						info.reference_level.resize(Eigen::NoChange, info.reference_time.size());
						for (size_t i = 0; i < info.reference_time.size(); i++)
							info.reference_level[i] = local_reference_level[i];

						mMax_Levels_Per_Segment = std::max(mMax_Levels_Per_Segment, info.reference_time.size())
						mSegment_Info.push_back(info);
					}
				}


				
			} //if we managed to get both segment and signal
		} //for each segments

	} //ctor's end

	double Calculate_Fitness(const TSolution &solution, glucose::SMetric &metric) const {

#ifdef D_PRINT_PARAMS
		stochastic_fitness::Print_Params(solution);
#endif

		//caller has to supply the metric to allow parallelization without inccuring an overhead
		//of creating new metric calculator instead of its mere resetting	
		metric->Reset();

		//let's pick a memory block for calculated
		TVector1D tmp_levels;
		if (!mTemporal_Levels.try_pop(tmp_levels)) {
			tmp_levels = std::make_unique<decltype(tmp_levels.get())>();	//create one if there is none left
			resize(Eigen::NoChange, mMax_Levels_Per_Segment);
		}



		for (auto &info : mSegment_Info) {
			size_t filled;
			if (info.signal->Get_Continuous_Levels(solution, info.reference_time.data(), tmp_levels.data(), info.reference_time.size(), &filled, glucose::apxNoDerivation) == S_OK) {
				//levels got, calculate the metric
			}
		}


		//finally, free the used l block
		mTemporal_Levels.push(tmp_levels);

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

};