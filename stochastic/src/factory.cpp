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
	virtual HRESULT Solve_Model_Parameters(const GUID &solver_id, const GUID &signal_id, const std::vector<glucose::STime_Segment> &segments, glucose::SMetric metric,
		const glucose::SModel_Parameter_Vector &lower_bound, const glucose::SModel_Parameter_Vector &upper_bound, glucose::SModel_Parameter_Vector &solved_parameters,
		const std::vector<glucose::SModel_Parameter_Vector> &solution_hints, glucose::TSolver_Progress &progress) = 0;
};


template <typename TSolution>
class CSpecilized_Id_Dispatcher : public CGeneral_Id_Dispatcher {

protected:
	std::map <const GUID, std::function<HRESULT(const GUID &signal_id, const std::vector<glucose::STime_Segment> &segments, glucose::SMetric metric,
		const glucose::SModel_Parameter_Vector &lower_bound, const glucose::SModel_Parameter_Vector &upper_bound, glucose::SModel_Parameter_Vector &solved_parameters,
		const std::vector<glucose::SModel_Parameter_Vector> &solution_hints, glucose::TSolver_Progress &progress)>> mSolver_Id_Map;
	
	template <typename  nlopt::algorithm algorithm_id>
	void add_NLOpt(const GUID &id) {
		mSolver_Id_Map[id] = std::bind(&CId_Dispatcher::Solve_NLOptalgorithm_id>, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, std::placeholders::_8);
	};

protected:
	template <typename  nlopt::algorithm algorithm_id>
	HRESULT Solve_NLOpt(const GUID &signal_id, const std::vector<glucose::STime_Segment> &segments, glucose::SMetric metric,
		const glucose::SModel_Parameter_Vector &lower_bound, const glucose::SModel_Parameter_Vector &upper_bound, glucose::SModel_Parameter_Vector &solved_parameters,
		const std::vector<glucose::SModel_Parameter_Vector> &solution_hints, glucose::TSolver_Progress &progress) const {
		
		using TFitness = CFitness<TSolution>;
		using TNLOpt_Specialized_Solver = CNLOpt<TSolution, TFitness, algorithm_id>;
		TFitness fitness;
		TNLOpt_Specialized_Solver solver(solution_hints, lower_bound, upper_bound, fitness, metric);
		return solver.solve(progress);
	}

public:
	CSpecilized_Id_Dispatcher() {	
		add_NLOpt<nlopt::LN_NEWUOA>(newuoa::id);		
		
	}

	virtual HRESULT Solve_Model_Parameters(const GUID &solver_id, const GUID &signal_id, const std::vector<glucose::STime_Segment> &segments, glucose::SMetric metric,
		const glucose::SModel_Parameter_Vector &lower_bound, const glucose::SModel_Parameter_Vector &upper_bound, glucose::SModel_Parameter_Vector &solved_parameters,
		const std::vector<glucose::SModel_Parameter_Vector> &solution_hints, glucose::TSolver_Progress &progress) final {

		const auto iter = mSolver_Id_Map.find(solver_id);
		if (iter != mSolver_Id_Map.end())
			return iter->second(signal_id, segments, metric, lower_bound, upper_bound, solved_parameters, solution_hints, progress);
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

	HRESULT Solve_Model_Parameters(const GUID &solver_id, const GUID &signal_id, const std::vector<glucose::STime_Segment> &segments, glucose::SMetric metric,
		const glucose::SModel_Parameter_Vector &lower_bound, const glucose::SModel_Parameter_Vector &upper_bound, glucose::SModel_Parameter_Vector &solved_parameters,
		const std::vector<glucose::SModel_Parameter_Vector> &solution_hints, glucose::TSolver_Progress &progress) {


		HRESULT rc;
		const auto iter = mSignal_Id_map.find(solver_id);
		if (iter != mSignal_Id_map.end()) rc = iter->second->Solve_Model_Parameters(solver_id, signal_id, segments, metric, lower_bound, upper_bound, solved_parameters, solution_hints, progress);
			else rc = mGeneric_Dispatcher.Solve_Model_Parameters(solver_id, signal_id, segments, metric, lower_bound, upper_bound, solved_parameters, solution_hints, progress);
		
		return rc;
	}
};

static CId_Dispatcher Id_Dispatcher;


HRESULT IfaceCalling do_solve_model_parameters(const GUID *solver_id, const GUID *signal_id, glucose::ITime_Segment **segments, const size_t segment_count, glucose::IMetric *metric,
											   glucose::IModel_Parameter_Vector *lower_bound, glucose::IModel_Parameter_Vector *upper_bound, glucose::IModel_Parameter_Vector **solved_parameters,
											   glucose::IModel_Parameter_Vector **solution_hints, const size_t hint_count, glucose::TSolver_Progress *progress) {
	
	if (segment_count == 0) return E_INVALIDARG;

	try {	//COM like steps do not throw Exceptions
		auto shared_segments = refcnt::Referenced_To_Vector<glucose::STime_Segment, glucose::ITime_Segment>(segments, segment_count);
		const auto shared_metric = refcnt::make_shared_reference<glucose::IMetric>(metric, true);
		const auto shared_lower = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(lower_bound, true);
		const auto shared_upper = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(upper_bound, true);
		auto shared_solved = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(*solved_parameters, true);
		auto shared_hints = refcnt::Referenced_To_Vector<glucose::SModel_Parameter_Vector, glucose::IModel_Parameter_Vector>(solution_hints, hint_count);

		//make sure that we do our best to supply at least one hint for local and evolutionary solvers
		auto default_parameters = refcnt::Create_Container_shared<double>(nullptr, nullptr);
		if (shared_hints.empty()) {
			//we need to try to obtain the default parameter at least
			auto signal = shared_segments[0].Get_Signal(*signal_id);
			if (signal) {
				if (signal->Get_Default_Parameters(default_parameters.get()) == S_OK)
					shared_hints.push_back(default_parameters);
			}
		}

		glucose::TSolver_Progress dummy_progress;
		if (progress == nullptr) progress = &dummy_progress;

		return Id_Dispatcher.Solve_Model_Parameters(*solver_id, *signal_id, shared_segments, shared_metric, shared_lower, shared_upper, shared_solved, shared_hints, *progress);
	}
	catch (...) {
		return E_FAIL;
	}
}