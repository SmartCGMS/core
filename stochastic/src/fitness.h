#pragma once

#include "../..\..\common\rtl\SolverLib.h"
#include "../..\..\common\rtl\Buffer_Pool.h"

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
	const GUID solver_id; const GUID signal_id;
	std::vector<glucose::STime_Segment> segments;
	glucose::SMetric metric;	const size_t levels_required; const char use_measured_levels;
	const glucose::SModel_Parameter_Vector lower_bound, upper_bound;
	const std::vector<glucose::SModel_Parameter_Vector> solution_hints;
	glucose::SModel_Parameter_Vector solved_parameters;
	glucose::TSolver_Progress &progress;
};


struct TSegment_Info {
	glucose::STime_Segment segment;
	glucose::SSignal signal;
	std::vector<double> reference_time;
	std::vector<double> reference_level;
};

template <typename TSolution>
class CFitness {
protected:
	std::vector<TSegment_Info> mSegment_Info;	
	size_t mLevels_Required;
	size_t mMax_Levels_Per_Segment;	//to avoid multiple resize of memory block when calculating the error
	CBuffer_Pool<std::vector<double>> mTemporal_Levels{ [](auto &container, auto minimum_size) {		
		if (container.size() < minimum_size) container.resize(minimum_size);
	} };
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
					info.reference_time.resize(levels_count);
					info.reference_level.resize(levels_count);					
					
					//prepare arrays with reference levels and their times
					if (info.signal->Get_Discrete_Levels(info.reference_time.data(), info.reference_level.data(), info.reference_time.size(), &levels_count) == S_OK) {
						info.reference_time.resize(levels_count);
						info.reference_level.resize(levels_count);
					}
					

					//if desired, replace them continous signal approdximation
					if (setup.use_measured_levels == 0) {
						info.signal->Get_Continuous_Levels(nullptr, info.reference_time.data(), info.reference_level.data(), info.reference_time.size(), glucose::apxNo_Derivation);
							//we are not interested in checking the possibly error, because we will be left with meeasured levels at least
					}

					//do we have everyting we need to test this segment?
					if (!info.reference_time.empty()) {
						mMax_Levels_Per_Segment = std::max(mMax_Levels_Per_Segment, info.reference_time.size());
						mSegment_Info.push_back(info);
					}
				}


				
			} //if we managed to get both segment and signal
		} //for each segments

	} //ctor's end

	double Calculate_Fitness(TSolution &solution, glucose::SMetric &metric) {

#ifdef D_PRINT_PARAMS
		stochastic_fitness::Print_Params(solution);
#endif

		//caller has to supply the metric to allow parallelization without inccuring an overhead
		//of creating new metric calculator instead of its mere resetting	
		metric->Reset();

		//let's pick a memory block for calculated
		CPooled_Buffer<std::vector<double>> tmp_levels = mTemporal_Levels.pop( mMax_Levels_Per_Segment );

		for (auto &info : mSegment_Info) {			
			if (info.signal->Get_Continuous_Levels(&solution, info.reference_time.data(), tmp_levels.element().data(), info.reference_time.size(), glucose::apxNo_Derivation) == S_OK) {
				//levels got, calculate the metric
				metric->Accumulate(info.reference_time.data(), info.reference_level.data(), tmp_levels.element().data(), info.reference_time.size());
			}
		}


		//eventually, calculate the metric number
		double result;
		size_t tmp_size;
		if (metric->Calculate(&result, &tmp_size, mLevels_Required) != S_OK) result = std::numeric_limits<double>::max();

		//additionaly, we need the glucose model to advertise a function that will provide number of model parameters
		//then, we could use this number as another metric for evolutionary programming to realize/bypass Paretto front

		return result;
	}

};