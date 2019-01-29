/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Copyright (c) since 2018 University of West Bohemia.
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Univerzitni 8
 * 301 00, Pilsen
 * 
 * 
 * Purpose of this software:
 * This software is intended to demonstrate work of the diabetes.zcu.cz research
 * group to other scientists, to complement our published papers. It is strictly
 * prohibited to use this software for diagnosis or treatment of any medical condition,
 * without obtaining all required approvals from respective regulatory bodies.
 *
 * Especially, a diabetic patient is warned that unauthorized use of this software
 * may result into severe injure, including death.
 *
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under these license terms is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *      GPLv3 license.
 * b) For any other use, especially commercial use, you must contact us and
 *       obtain specific terms and conditions for the use of the software.
 * c) When publishing work with results obtained using this software, you agree to cite the following paper:
 *       Tomas Koutny and Martin Ubl, "Parallel software architecture for the next generation of glucose
 *       monitoring", Procedia Computer Science, Volume 141C, pp. 279-286, 2018
 */

#include "factory.h"

#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include "NLOpt.h"
#include "MetaDE.h"
#include "NullMethod.h"
#include "HaltonSequence.h"
#include "DeterministicEvolution.h"
#include "pagmo2.h"

#include "descriptor.h"
#include "fitness.h"
#include "solution.h"

#include <map>
#include <functional>

#include <tbb/tbb_allocator.h>

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
HRESULT Solve_By_Class(TShared_Solver_Setup &setup, TAligned_Solution_Vector<TSolution>& hints, const TSolution& lower_bound, const TSolution& upper_bound) /*const */{

	TFitness fitness{ setup };
	TSolver  solver{ hints, lower_bound, upper_bound, fitness, setup.metric };

	TSolution solved = solver.Solve(setup.progress);
	Solution_To_Parameters(solved, setup.solved_parameters);

	return S_OK;
}

template <typename TSolution, nlopt::algorithm algorithm_id>
HRESULT Solve_NLOpt(TShared_Solver_Setup &setup, TAligned_Solution_Vector<TSolution>& hints, const TSolution& lower_bound, const TSolution& upper_bound)  {

	using TFitness = CFitness<TSolution>;
	using TNLOpt_Specialized_Solver = CNLOpt<TSolution, TFitness, algorithm_id>;
	TFitness fitness{ setup };
	TNLOpt_Specialized_Solver solver{ hints, lower_bound, upper_bound, fitness, setup.metric };

	TSolution solved = solver.Solve(setup.progress);
	Solution_To_Parameters(solved, setup.solved_parameters);
	
	return S_OK;
}

template <typename TSolution>
using  TSolve = std::function<HRESULT(TShared_Solver_Setup &, TAligned_Solution_Vector<TSolution>&, const TSolution&, const TSolution&)>;

template <typename TSolution, typename TFitness = CFitness<TSolution>>
class CSpecialized_Id_Dispatcher : public CGeneral_Id_Dispatcher {
protected:
	TSolution Parameters_To_Solution(const glucose::SModel_Parameter_Vector &parameters) const {
		TSolution solution;

		double *begin, *end;
		if (parameters->get(&begin, &end) == S_OK) {
			solution.resize(solution.rows(), std::distance(begin, end));

			for (auto i = 0; i < solution.cols(); i++) {
				solution[i] = *begin;
				begin++;
			}
		} 

		return solution;
	}

protected:
	std::map <const GUID, TSolve<TSolution>, std::less<GUID>, tbb::tbb_allocator<std::pair<const GUID, TSolve<TSolution>>>> mSolver_Id_Map;

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
		add_NLOpt<nlopt::LN_BOBYQA>(bobyqa::id);

		using TMT_MetaDE = CMetaDE<TSolution, TFitness, CNullMethod<TSolution, TFitness>, 100000, 100, std::mt19937>;
		mSolver_Id_Map[mt_metade::id] = std::bind(&Solve_By_Class<TMT_MetaDE, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		
		using THalton_MetaDE = CMetaDE<TSolution, TFitness, CNullMethod<TSolution, TFitness>, 100000, 100, CHalton_Device>;
		mSolver_Id_Map[halton_metade::id] = std::bind(&Solve_By_Class<THalton_MetaDE, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

		using TRandom_MetaDE = CMetaDE<TSolution, TFitness, CNullMethod<TSolution, TFitness>, 100000, 100, std::random_device>;
		mSolver_Id_Map[rnd_metade::id] = std::bind(&Solve_By_Class<TRandom_MetaDE, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

		mSolver_Id_Map[halton_sequence::id] = std::bind(&Solve_By_Class<CHalton_Sequence<TSolution, TFitness>, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
		
		mSolver_Id_Map[deterministic_evolution::id] = std::bind(&Solve_By_Class<CDeterministic_Evolution<TSolution, TFitness>, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

		mSolver_Id_Map[pso::id] = std::bind(&Solve_By_Class<CPagmo2<TSolution, TFitness, pagmo2::EPagmo_Algo::PSO>, TSolution, TFitness>, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	}

	virtual HRESULT Solve_Model_Parameters(TShared_Solver_Setup &setup) final {

		TAligned_Solution_Vector<TSolution> converted_hints;
		for (auto &hint : setup.solution_hints) {
			if (hint->empty() != S_OK) {
				auto converted = Parameters_To_Solution(hint);
				converted_hints.push_back(converted);
			}
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
	CSpecialized_Id_Dispatcher<CSteil_Rebrin_Diffusion_Prediction_Solution> mSteil_Rebrin_Diffusion_Prediction_Dispatcher;
	CSpecialized_Id_Dispatcher<CDiffusion_Prediction_Solution> mDiffusion_Prediction_Dispatcher;
	CSpecialized_Id_Dispatcher<CGeneric_Solution> mGeneric_Dispatcher;
	std::map<const GUID, CGeneral_Id_Dispatcher*, std::less<GUID>, tbb::tbb_allocator<std::pair<const GUID, CGeneral_Id_Dispatcher*>>> mSignal_Id_map;
public:
	CId_Dispatcher() {
		//alternatively, we could call factory.dll and enumerate signals for known models
		//however, this code is much simpler right now
		mSignal_Id_map[glucose::signal_Diffusion_v2_Blood] = &mDiffusion_v2_Dispatcher;
		mSignal_Id_map[glucose::signal_Diffusion_v2_Ist] = &mDiffusion_v2_Dispatcher;
		mSignal_Id_map[glucose::signal_Steil_Rebrin_Blood] = &mSteil_Rebrin_Dispatcher;
		mSignal_Id_map[glucose::signal_Steil_Rebrin_Diffusion_Prediction] = &mSteil_Rebrin_Diffusion_Prediction_Dispatcher;
		mSignal_Id_map[glucose::signal_Diffusion_Prediction] = &mDiffusion_Prediction_Dispatcher;
	}

	HRESULT Solve_Model_Parameters(TShared_Solver_Setup &setup) {
		HRESULT rc;
		const auto iter = mSignal_Id_map.find(setup.calculated_signal_id);
		if (iter != mSignal_Id_map.end()) rc = iter->second->Solve_Model_Parameters(setup);
			else rc = mGeneric_Dispatcher.Solve_Model_Parameters(setup);
		
		return rc;
	}
};

static CId_Dispatcher Id_Dispatcher;


HRESULT IfaceCalling do_solve_model_parameters(const glucose::TSolver_Setup *setup) {
	
	if (setup->segment_count == 0) return E_INVALIDARG;
	if (setup->metric == nullptr) return E_INVALIDARG;

	try {	//COM like steps do not throw Exceptions
		auto shared_segments = refcnt::Referenced_To_Vector<glucose::STime_Segment, glucose::ITime_Segment>(setup->segments, setup->segment_count);
		const glucose::SMetric shared_metric = refcnt::make_shared_reference_ext<glucose::SMetric, glucose::IMetric>(setup->metric, true);
		const auto shared_lower = refcnt::make_shared_reference_ext<glucose::SModel_Parameter_Vector, glucose::IModel_Parameter_Vector>(setup->lower_bound, true);
		const auto shared_upper = refcnt::make_shared_reference_ext<glucose::SModel_Parameter_Vector, glucose::IModel_Parameter_Vector>(setup->upper_bound, true);
		auto shared_solved = refcnt::make_shared_reference_ext<glucose::SModel_Parameter_Vector, glucose::IModel_Parameter_Vector>(setup->solved_parameters, true);
		auto shared_hints = refcnt::Referenced_To_Vector<glucose::SModel_Parameter_Vector, glucose::IModel_Parameter_Vector>(setup->solution_hints, setup->hint_count);

		//make sure that we do our best to supply at least one hint for local and evolutionary solvers
		auto default_parameters = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(nullptr, nullptr);
		if (shared_hints.empty()) {
			//we need to try to obtain the default parameter at least
			auto signal = shared_segments[0].Get_Signal(setup->calculated_signal_id);
			if (signal) {
				if (signal->Get_Default_Parameters(default_parameters.get()) == S_OK)
					shared_hints.push_back(default_parameters);
			}
		}

		glucose::TSolver_Progress dummy_progress = glucose::Null_Solver_Progress;
		glucose::TSolver_Progress* local_progress = setup->progress;
		if (local_progress == nullptr) local_progress = &dummy_progress;


		const auto const_hints{ shared_hints };

		TShared_Solver_Setup shared_setup {
			setup->solver_id, setup->calculated_signal_id, setup->reference_signal_id,
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
