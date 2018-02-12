#include "factory.h"

#include "../../../common/rtl/SolverLib.h"

#include "NLOpt.h"
#include "descriptor.h"
#include "fitness.h"
#include "solution.h"

#include <map>
#include <functional>

/*

virtual HRESULT set(const double *begin, const double *end) final { \
for (size_t i=0; i<cols(); i++) { \
operator[](i) = *begin; \
begin++; \
} \
return S_OK; \
} \
\
virtual HRESULT get(double **begin, double **end) const final { \
*begin = data(); \
*end = *begin+cols(); \
return S_OK; \
} \
\
static derived_type From_Parameter_Vector(glucose::SModel_Parameter_Vector &vector) const { \
derived_type result; \
double *begin, *end; \
if (ColsAtCompileTime() == Eigen::Dynamic) result.resize(Eigen::NoChange, std::distance(begin, end)); \
if (vector->get(&begin, &end) == S_OK) \
result.set(begin, end); \
return result; \
} \
\
void Set_To_Vector(glucose::SModel_Parameter_Vector &vector) const { \
vector->set(data(), data+cols()); \
}

*/



class CGeneral_Id_Dispatcher {
public:
	virtual HRESULT Solve_Model_Parameters(TShared_Solver_Setup &setup) = 0;
};


template <typename TSolution>
class CSpecilized_Id_Dispatcher : public CGeneral_Id_Dispatcher {

protected:
	std::map <const GUID, std::function<HRESULT(TShared_Solver_Setup &setup)>> mSolver_Id_Map;
	
	template <typename  nlopt::algorithm algorithm_id>
	void add_NLOpt(const GUID &id) {
		mSolver_Id_Map[id] = std::bind(&CId_Dispatcher::Solve_NLOptalgorithm_id>, this, std::placeholders::_1);
	};

protected:
	template <typename  nlopt::algorithm algorithm_id>
	HRESULT Solve_NLOpt(TShared_Solver_Setup &setup) const {
		
		using TFitness = CFitness<TSolution>;
		using TNLOpt_Specialized_Solver = CNLOpt<TSolution, TFitness, algorithm_id>;
		TFitness fitness;
		TNLOpt_Specialized_Solver solver(setup.solution_hints, setup.lower_bound, setup.upper_bound, fitness, setup.metric, setup.progress);
		return solver.solve(progress);
	}

public:
	CSpecilized_Id_Dispatcher() {	
		add_NLOpt<nlopt::LN_NEWUOA>(newuoa::id);		
		
	}

	virtual HRESULT Solve_Model_Parameters(TShared_Solver_Setup &setup) final {

		const auto iter = mSolver_Id_Map.find(setup.solver_id);
		if (iter != mSolver_Id_Map.end())
			return iter->second(setup);
			else return E_NOTIMPL;
	}
};


class CId_Dispatcher {
protected:
	CSpecilized_Id_Dispatcher<CDiffusion_v2_Solution> mDiffusion_v2_Dispatcher;
	CSpecilized_Id_Dispatcher<CSteil_Rebrin_Solution> mSteil_Rebrin_Dispatcher;
	CSpecilized_Id_Dispatcher<CGeneric_Solution> mGeneric_Dispatcher;
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
		const auto iter = mSignal_Id_map.find(setup.solver_id);
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
		const auto shared_metric = refcnt::make_shared_reference<glucose::IMetric>(setup->metric, true);
		const auto shared_lower = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(setup->lower_bound, true);
		const auto shared_upper = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(setup->upper_bound, true);
		auto shared_solved = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(*(setup->solved_parameters), true);
		auto shared_hints = refcnt::Referenced_To_Vector<glucose::SModel_Parameter_Vector, glucose::IModel_Parameter_Vector>(setup->solution_hints, setup->hint_count);

		//make sure that we do our best to supply at least one hint for local and evolutionary solvers
		auto default_parameters = refcnt::Create_Container_shared<double>(nullptr, nullptr);
		if (shared_hints.empty()) {
			//we need to try to obtain the default parameter at least
			auto signal = shared_segments[0].Get_Signal(*(setup->signal_id));
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