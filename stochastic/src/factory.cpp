#include "factory.h"

#include "NLOpt.h"
#include "descriptor.h"

#include "skeleton.h"
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



class CId_Dispatcher {
protected:
	
protected:
	std::map <const GUID, std::function<HRESULT(const GUID &signal_id, const std::vector<glucose::STime_Segment> &segments, glucose::SMetric metric,
		const glucose::SModel_Parameter_Vector &lower_bound, const glucose::SModel_Parameter_Vector &upper_bound, glucose::SModel_Parameter_Vector &solved_parameters,
		glucose::TSolver_Progress &progress)>> id_map;
	
	template <nlopt::algorithm algorithm_id>
	HRESULT Solve_NLOpt(const GUID &signal_id, const std::vector<glucose::STime_Segment> &segments, glucose::SMetric metric,
		const glucose::SModel_Parameter_Vector &lower_bound, const glucose::SModel_Parameter_Vector &upper_bound, glucose::SModel_Parameter_Vector &solved_parameters,
		glucose::TSolver_Progress &progress) const {
		
		auto CSolver_Skeleton<CNLOpt<CGeneric_Solution, TGeneric_Vector, algorithm_id>, CGeneric_Solution, TDiffusion2_Fitness>, ISolver>(solver, segments, segmentcount, metricfactory, cidDiffusion2Blood);

		return E_NOTIMPL;
	}

public:
	CId_Dispatcher() {
		id_map[newuoa::id] = std::bind(&CId_Dispatcher::Solve_NLOpt<nlopt::LN_NEWUOA>, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7);
		
	}

	HRESULT Solve_Model_Parameters(const GUID &solver_id, const GUID &signal_id, const std::vector<glucose::STime_Segment> &segments, glucose::SMetric metric,
		const glucose::SModel_Parameter_Vector &lower_bound, const glucose::SModel_Parameter_Vector &upper_bound, glucose::SModel_Parameter_Vector &solved_parameters,
		glucose::TSolver_Progress &progress) {

		const auto iter = id_map.find(solver_id);
		if (iter != id_map.end())
			return iter->second(signal_id, segments, metric, lower_bound, upper_bound, solved_parameters, progress);
			else return E_NOTIMPL;
	}
};



static CId_Dispatcher Id_Dispatcher;


HRESULT IfaceCalling do_solve_model_parameters(const GUID *solver_id, const GUID *signal_id, const glucose::ITime_Segment **segments, const size_t segment_count, glucose::IMetric *metric,
											   glucose::IModel_Parameter_Vector *lower_bound, glucose::IModel_Parameter_Vector *upper_bound, glucose::IModel_Parameter_Vector **solved_parameters,
											   glucose::TSolver_Progress *progress) {
	
	
	const auto shared_segments = glucose::Time_Segments_To_Vector(segments, segment_count);
	const auto shared_metric = refcnt::make_shared_reference<glucose::IMetric>(metric, true);
	const auto shared_lower = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(lower_bound, true);
	const auto shared_upper = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(upper_bound, true);
	auto shared_solved = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(*solved_parameters, true);

	glucose::TSolver_Progress dummy_progress;

	return Id_Dispatcher.Solve_Model_Parameters(*solver_id, *signal_id, shared_segments, shared_metric, shared_lower, shared_upper, shared_solved, progress != nullptr ? *progress : dummy_progress);
}