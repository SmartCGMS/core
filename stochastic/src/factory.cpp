#include "factory.h"

#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include "NLOpt.h"
#include "MetaDE.h"
#include "NullMethod.h"

#include "descriptor.h"
#include "fitness.h"
#include "solution.h"

#include <map>
#include <functional>

class CGeneral_Id_Dispatcher {
public:
	virtual HRESULT Solve_Model_Parameters(TShared_Solver_Setup &setup) = 0;
};


template <typename TSolution>
void Solution_To_Parameters(TSolution &src, glucose::SModel_Parameter_Vector dst) {
	double *begin = src.data();
	double *end = begin + src.cols();
	dst->set(begin, end);
}

template <typename TSolver, typename TSolution, typename TFitness>
HRESULT Solve_By_Class(TShared_Solver_Setup &setup, std::vector<TSolution>& hints, const TSolution& lower_bound, const TSolution& upper_bound) /*const */{

	TFitness fitness{ setup };
	TSolver  solver{ hints, lower_bound, upper_bound, fitness, setup.metric };

	TSolution solved = solver.Solve(setup.progress);
	Solution_To_Parameters(solved, setup.solved_parameters);

	return S_OK;
}

template <typename TSolution, nlopt::algorithm algorithm_id>
HRESULT Solve_NLOpt(TShared_Solver_Setup &setup, std::vector<TSolution>& hints, const TSolution& lower_bound, const TSolution& upper_bound)  {

	using TFitness = CFitness<TSolution>;
	using TNLOpt_Specialized_Solver = CNLOpt<TSolution, TFitness, algorithm_id>;
	TFitness fitness{ setup };
	TNLOpt_Specialized_Solver solver{ hints, lower_bound, upper_bound, fitness, setup.metric };

	TSolution solved = solver.Solve(setup.progress);
	Solution_To_Parameters(solved, setup.solved_parameters);

	return S_OK;
}

template <typename TSolution, typename TFitness = CFitness<TSolution>>
class CSpecialized_Id_Dispatcher : public CGeneral_Id_Dispatcher {
protected:
	TSolution Parameters_To_Solution(const glucose::SModel_Parameter_Vector &parameters) const {
		TSolution solution;

		double *begin, *end;				
		if (parameters->get(&begin, &end) == S_OK) 
			solution.resize(solution.rows(), std::distance(begin, end));

			for (auto i = 0; i<solution.cols(); i++) {
				solution[i] = *begin; 
				begin++; 
			} 

		return solution;
	}
	
protected:
	std::map <const GUID, std::function<HRESULT(TShared_Solver_Setup &, std::vector<TSolution>&, const TSolution&, const TSolution&)>> mSolver_Id_Map;

	template <nlopt::algorithm algorithm_id>
	void add_NLOpt(const GUID &id) {		
		mSolver_Id_Map[id] = std::bind(&Solve_NLOpt<TSolution, algorithm_id>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	}

protected:
	//Here, we used to have Solution_To_Parameters, Solve_By_Class, and Solve_NLOpt.
	//They used to compile fine with MSVC'17, ICC'18 but got broken with ICC'19.
	//Template at the method level was the problem. So, not to throw away the benefit of ICC optimizations, we rather took those const methods outside the class.

	
public:
	CSpecialized_Id_Dispatcher() {	
		add_NLOpt<nlopt::LN_NEWUOA>(newuoa::id);		

		using TPure_MetaDE = CMetaDE<TSolution, TFitness, CNullMethod<TSolution, TFitness>>;		
		mSolver_Id_Map[metade::id] = std::bind(&Solve_By_Class<TPure_MetaDE, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		
	}

	virtual HRESULT Solve_Model_Parameters(TShared_Solver_Setup &setup) final {

		std::vector<TSolution> converted_hints;
		for (auto &hint : setup.solution_hints) {
			auto converted = Parameters_To_Solution(hint);
			converted_hints.push_back(converted);
		}

		TSolution convert_lower_bound = Parameters_To_Solution(setup.lower_bound);
		TSolution convert_upper_bound = Parameters_To_Solution(setup.upper_bound);

		const auto iter = mSolver_Id_Map.find(setup.solver_id);
		if (iter != mSolver_Id_Map.end())
			return iter->second(setup, converted_hints, convert_lower_bound, convert_upper_bound);
			else return E_NOTIMPL;
	}
};


class CId_Dispatcher {
protected:
	CSpecialized_Id_Dispatcher<CDiffusion_v2_Solution> mDiffusion_v2_Dispatcher;
	CSpecialized_Id_Dispatcher<CSteil_Rebrin_Solution> mSteil_Rebrin_Dispatcher;
	CSpecialized_Id_Dispatcher<CGeneric_Solution> mGeneric_Dispatcher;
	std::map<const GUID, CGeneral_Id_Dispatcher*> mSignal_Id_map;
public:
	CId_Dispatcher() {
		//alternatively, we could call factory.dll and enumerate signals for known models
		//however, this code is much simpler right now
		mSignal_Id_map[glucose::signal_Diffusion_v2_Blood] = &mDiffusion_v2_Dispatcher;
		mSignal_Id_map[glucose::signal_Diffusion_v2_Ist] = &mDiffusion_v2_Dispatcher;
		mSignal_Id_map[glucose::signal_Steil_Rebrin_Blood] = &mSteil_Rebrin_Dispatcher;
	}

	HRESULT Solve_Model_Parameters(TShared_Solver_Setup &setup) {
		HRESULT rc;
		const auto iter = mSignal_Id_map.find(setup.signal_id);
		if (iter != mSignal_Id_map.end()) rc = iter->second->Solve_Model_Parameters(setup);
			else rc = mGeneric_Dispatcher.Solve_Model_Parameters(setup);
		
		return rc;
	}
};

static CId_Dispatcher Id_Dispatcher;


HRESULT IfaceCalling do_solve_model_parameters(const glucose::TSolver_Setup *setup) {
	
	if (setup->segment_count == 0) return E_INVALIDARG;

	try {	//COM like steps do not throw Exceptions
		auto shared_segments = refcnt::Referenced_To_Vector<glucose::STime_Segment, glucose::ITime_Segment>(setup->segments, setup->segment_count);
		const glucose::SMetric shared_metric = refcnt::make_shared_reference_ext<glucose::SMetric, glucose::IMetric>(setup->metric, true);
		const auto shared_lower = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(setup->lower_bound, true);
		const auto shared_upper = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(setup->upper_bound, true);
		auto shared_solved = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(*(setup->solved_parameters), true);
		auto shared_hints = refcnt::Referenced_To_Vector<glucose::SModel_Parameter_Vector, glucose::IModel_Parameter_Vector>(setup->solution_hints, setup->hint_count);

		//make sure that we do our best to supply at least one hint for local and evolutionary solvers
		auto default_parameters = refcnt::Create_Container_shared<double>(nullptr, nullptr);
		if (shared_hints.empty()) {
			//we need to try to obtain the default parameter at least
			auto signal = shared_segments[0].Get_Signal(setup->signal_id);
			if (signal) {
				if (signal->Get_Default_Parameters(default_parameters.get()) == S_OK)
					shared_hints.push_back(default_parameters);
			}
		}

		glucose::TSolver_Progress dummy_progress;
		glucose::TSolver_Progress* local_progress = setup->progress;
		if (local_progress == nullptr) local_progress = &dummy_progress;


		const auto const_hints{ shared_hints };

		TShared_Solver_Setup shared_setup {
			setup->solver_id, setup->signal_id,
			shared_segments,
			shared_metric, setup->levels_required, setup->use_measured_levels,
			shared_lower, shared_upper,
			const_hints,
			shared_solved,
			*local_progress
		};
		
		return Id_Dispatcher.Solve_Model_Parameters(shared_setup);
	}
	catch (...) {
		return E_FAIL;
	}
}