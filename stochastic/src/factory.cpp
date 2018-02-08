#include "factory.h"


#include "descriptor.h"

#include <map>
#include <functional>


#include "../../../common/rtl/DeviceLib.h"
#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/manufactory.h"


class CId_Dispatcher {
protected:
	std::map <const GUID, std::function<HRESULT(const GUID &signal_id, const std::vector<glucose::STime_Segment> &segments, glucose::SMetric metric,
		const glucose::SModel_Parameter_Vector &lower_bound, const glucose::SModel_Parameter_Vector &upper_bound, glucose::SModel_Parameter_Vector &solved_parameters)>> id_map;
	
	HRESULT Solve_NewUOA(const GUID &signal_id, const std::vector<glucose::STime_Segment> &segments, glucose::SMetric metric,
		const glucose::SModel_Parameter_Vector &lower_bound, const glucose::SModel_Parameter_Vector &upper_bound, glucose::SModel_Parameter_Vector &solved_parameters) const {
		
		return E_NOTIMPL;
	}

public:
	CId_Dispatcher() {
		id_map[newuoa::id] = std::bind(&CId_Dispatcher::Solve_NewUOA, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
		
	}

	HRESULT Solve_Model_Parameters(const GUID &solver_id, const GUID &signal_id, const std::vector<glucose::STime_Segment> &segments, glucose::SMetric metric,
		const glucose::SModel_Parameter_Vector &lower_bound, const glucose::SModel_Parameter_Vector &upper_bound, glucose::SModel_Parameter_Vector &solved_parameters) {

		const auto iter = id_map.find(solver_id);
		if (iter != id_map.end())
			return iter->second(signal_id, segments, metric, lower_bound, upper_bound, solved_parameters);
		else return E_NOTIMPL;
	}
};



static CId_Dispatcher Id_Dispatcher;


HRESULT IfaceCalling do_solve_model_parameters(const GUID *solver_id, const GUID *signal_id, const glucose::ITime_Segment **segments, const size_t segment_count, glucose::IMetric *metric,
											   glucose::IModel_Parameter_Vector *lower_bound, glucose::IModel_Parameter_Vector *upper_bound, glucose::IModel_Parameter_Vector **solved_parameters) {
	
	
	const auto shared_segments = glucose::Time_Segments_To_Vector(segments, segment_count);
	const auto shared_metric = refcnt::make_shared_reference<glucose::IMetric>(metric, true);
	const auto shared_lower = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(lower_bound, true);
	const auto shared_upper = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(upper_bound, true);
	auto shared_solved = refcnt::make_shared_reference<glucose::IModel_Parameter_Vector>(*solved_parameters, true);

	return Id_Dispatcher.Solve_Model_Parameters(*solver_id, *signal_id, shared_segments, shared_metric, shared_lower, shared_upper, shared_solved);
}