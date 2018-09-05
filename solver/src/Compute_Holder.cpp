/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#include "Compute_Holder.h"

#include "../../../common/rtl/ModelsLib.h"
#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/winapi_mapping.h"

#include <cmath>

CCompute_Holder::CCompute_Holder(const GUID &solver_id, const GUID &signal_id, const glucose::TMetric_Parameters &metric_parameters, const size_t metric_levels_required, const char use_measured_levels,
	bool use_just_opened_segments)
	: mSolveInProgress(false), mSolverId(solver_id), mSignalId(signal_id), mMetric{ metric_parameters }, mMetricLevelsRequired(metric_levels_required), mUseMeasuredLevels(use_measured_levels),
	  mLowBounds(), mHighBounds(), mLastStoppedSegment(0), mDetermine_Parameters_Using_All_Known_Segments(use_just_opened_segments), mMaxTime(0.0)
{
	//
}

void CCompute_Holder::Start_Segment(uint64_t segment_id)
{
	// manufacture a new object, add reference
	glucose::CTime_Segment *a;
	Manufacture_Object<glucose::CTime_Segment>(&a);

	glucose::SModel_Parameter_Vector params;
	if (mLastStoppedSegment && mSegments[mLastStoppedSegment].parameters)
	{
		params = mSegments[mLastStoppedSegment].parameters;
	}
	else if (mDefaultParameters) // fill user-defined default parameters
	{
		params = mDefaultParameters;
	}
	else // fill default model parameters
	{
		glucose::TModel_Descriptor desc = glucose::Null_Model_Descriptor;

		if (glucose::get_model_descriptor_by_signal_id(mSignalId, desc)) {
			double *default_params_ptr = const_cast<double*>(desc.default_values);
			params = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(default_params_ptr, default_params_ptr + desc.number_of_parameters);
		}
	}

	mSegments[segment_id] = {
		refcnt::make_shared_reference_ext<glucose::STime_Segment, glucose::CTime_Segment>(a, false),
		params,
		true
	};
}

void CCompute_Holder::Stop_Segment(uint64_t segment_id)
{
	auto itr = mSegments.find(segment_id);
	if (itr == mSegments.end() || !itr->second.opened)
		return;

	itr->second.opened = false;
	mLastStoppedSegment = segment_id;
}

bool CCompute_Holder::Add_Level(uint64_t segment_id, const GUID& signal_id, double time, double level)
{
	auto itr = mSegments.find(segment_id);
	if (itr == mSegments.end() || !itr->second.opened)
		return false;

	auto seg = itr->second.segment;

	auto signal = seg.Get_Signal(signal_id);
	if (!signal)
		return false;

	if (signal->Add_Levels(&time, &level, 1) != S_OK)
		return false;

	if (time > mMaxTime)
		mMaxTime = time;

	return true;
}

double CCompute_Holder::Get_Max_Time() const
{
	return mMaxTime;
}

void CCompute_Holder::Add_Solution_Hint(glucose::SModel_Parameter_Vector parameters)
{
	// copy parameter hint to internal vector
	mSolutionHints.push_back(refcnt::Copy_Container<double, glucose::SModel_Parameter_Vector>(parameters));
}

void CCompute_Holder::Set_Bounds(glucose::SModel_Parameter_Vector low, glucose::SModel_Parameter_Vector high)
{
	mLowBounds = low;
	mHighBounds = high;
}

void CCompute_Holder::Set_Defaults(glucose::SModel_Parameter_Vector defaults)
{
	mDefaultParameters = defaults;
}

bool CCompute_Holder::Fill_Default_Model_Bounds(const GUID &calculated_signal_id, GUID &reference_signal_id, glucose::SModel_Parameter_Vector &low, glucose::SModel_Parameter_Vector &defaults, glucose::SModel_Parameter_Vector &high)
{
	glucose::TModel_Descriptor desc = glucose::Null_Model_Descriptor;
	const bool result = glucose::get_model_descriptor_by_signal_id(calculated_signal_id, desc);

	if (result) {
		//find the proper reference id
		reference_signal_id = Invalid_GUID;	//sanity check
		for (size_t i=0; i<desc.number_of_calculated_signals; i++)
			if (desc.calculated_signal_ids[i] == calculated_signal_id) {
				reference_signal_id = desc.reference_signal_ids[i];
				break;
			}

		low = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(const_cast<double*>(desc.lower_bound), const_cast<double*>(desc.lower_bound) + desc.number_of_parameters);
		high = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(const_cast<double*>(desc.upper_bound), const_cast<double*>(desc.upper_bound) + desc.number_of_parameters);
		defaults = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(const_cast<double*>(desc.default_values), const_cast<double*>(desc.default_values) + desc.number_of_parameters);
	}

	return result;
}

glucose::TSolver_Setup CCompute_Holder::Prepare_Solver_Setup() {	
	GUID reference_signal_id;
	Fill_Default_Model_Bounds(mSignalId, reference_signal_id, mLowBounds, mDefaultParameters, mHighBounds);

	mClonedSegmentIds.clear();
	// create ITime_Segment pointer to stored segments since double indirection is not polymorphic in standard way
	for (auto& seg : mSegments)
	{
		if (!seg.second.opened && !mDetermine_Parameters_Using_All_Known_Segments)
			continue;

		if (auto sharedSegment = std::dynamic_pointer_cast<glucose::CTime_Segment>(seg.second.segment))
		{
			mClonedSegments.push_back(sharedSegment->Clone());
			mClonedSegmentIds.push_back(seg.first);
		}
	}

	// if no segments are opened and we request solving, use last segment
	if (!mDetermine_Parameters_Using_All_Known_Segments && mClonedSegments.empty() && mLastStoppedSegment != 0)
	{
		if (auto sharedSegment = std::dynamic_pointer_cast<glucose::CTime_Segment>(mSegments[mLastStoppedSegment].segment))
		{
			mClonedSegments.push_back(sharedSegment->Clone());
			mClonedSegmentIds.push_back(mLastStoppedSegment);
		}
	}

	TSolution_Hint_Vector hints = mSolutionHints;
	if (hints.empty())
	{
		if (mDefaultParameters)
			hints.push_back(mDefaultParameters);
	}

	// for interfacing with solver we need to store contents of shared pointers anyway, but we don't manually control reference counting;
	// the object is released with shared_ptr destruction, and the container (copied out) structures are guaranteed to be valid during solve

	mTmpSolverContainer.clonedSegments.clear();
	mTmpSolverContainer.solutionHints.clear();

	for (auto segment : mClonedSegments)
		mTmpSolverContainer.clonedSegments.push_back(segment.get());
	for (auto hint : mSolutionHints)
		mTmpSolverContainer.solutionHints.push_back(hint.get());

	mTempModelParams = refcnt::Create_Container_shared<double, glucose::SModel_Parameter_Vector>(nullptr, nullptr);
	mTmpSolverContainer.paramsTarget = mTempModelParams.get();

	mSolverProgress.cancelled = 0;


	// create solver setup structure
	return {
		mSolverId,																// solver_id
		mSignalId,																// calculated signal_id
		reference_signal_id,
		mTmpSolverContainer.clonedSegments.data(), mTmpSolverContainer.clonedSegments.size(),	// segments, segment_count
		mMetric.get(), mMetricLevelsRequired, static_cast<unsigned char>(mUseMeasuredLevels),				// metric, levels_required, use_measured_levels
		mLowBounds.get(), mHighBounds.get(),									// lower_bound, upper_bound
		mTmpSolverContainer.solutionHints.data(), mTmpSolverContainer.solutionHints.size(),		// solution_hints, hint_count
		mTmpSolverContainer.paramsTarget,										// solved_parameters
		&mSolverProgress														// progress
	};
}

HRESULT CCompute_Holder::Solve(const glucose::TSolver_Setup &solverSetup)
{
	// start the solver
	HRESULT rc = glucose::Solve_Model_Parameters(solverSetup);

	if (rc == S_OK)
	{
		mImprovedSegmentIds.clear();

		// compare solutions using metric on current (cloned) dataset
		if (Compare_Solutions(mMetric))
		{
			for (uint64_t segId : mClonedSegmentIds)
			{
				// replace with new parameters
				mSegments[segId].parameters = mTempModelParams;
				mImprovedSegmentIds.push_back(segId);
			}
		}

		if (mImprovedSegmentIds.empty()) // new parameters are worse for every segment
		{
			// set return code to S_FALSE --> we failed to find a better solution, so we won't propagate parameters, recalculate signal, ..
			rc = S_FALSE;
		}
	}

	mTempModelParams.reset();

	mClonedSegments.clear();
	mClonedSegmentIds.clear();	

	mSolveInProgress = false;

	return rc;
}

std::future<HRESULT> CCompute_Holder::Solve_Async() {
	if (mSolveInProgress)
		return std::future<HRESULT>();

	mSolveInProgress = true;

	glucose::TSolver_Setup setup = Prepare_Solver_Setup();

	return std::async(std::launch::async, [&, setup]() -> HRESULT {
		return Solve(setup);
	});
}

bool CCompute_Holder::Compare_Solutions(glucose::SMetric metric) {

	// no solution found - then it's not better
	if (!mTempModelParams)
		return false;

	glucose::TModel_Descriptor desc = glucose::Null_Model_Descriptor;
	if (!glucose::get_model_descriptor_by_signal_id(mSignalId, desc)) return false;
	GUID reference_signal_id = Invalid_GUID;	//sanity check
	for (size_t i = 0; i<desc.number_of_calculated_signals; i++)
		if (desc.calculated_signal_ids[i] == mSignalId) {
			reference_signal_id = desc.reference_signal_ids[i];
			break;
		}


	std::vector<glucose::SSignal> calcSignals(mClonedSegmentIds.size());

	
	// all segments and their extracted times, levels and calculated levels
	std::vector<std::vector<double>> allRefTimes(mClonedSegmentIds.size());
	std::vector<std::vector<double>> allRefLevels(mClonedSegmentIds.size());
	std::vector<std::vector<double>> allCalcLevels(mClonedSegmentIds.size());

	metric->Reset();

	// accumulate metric across all cloned segments using old parameters
	for (size_t idx = 0; idx < mClonedSegmentIds.size(); idx++)
	{
		glucose::STime_Segment segment = mClonedSegments[idx];
		auto calcSignal = segment.Get_Signal(mSignalId);
		auto reference_signal = segment.Get_Signal(reference_signal_id);
		

		// get level count
		size_t levels_count;
		if (!calcSignal)
			return false;
		if (reference_signal->Get_Discrete_Bounds(nullptr, &levels_count) != S_OK)
			continue;

		auto& refTime = allRefTimes[idx];
		auto& refLevels = allRefLevels[idx];
		refTime.resize(levels_count);
		refLevels.resize(levels_count);

		// retrieve measured signal (if it's calculated signal, it will fall through to reference signal, which is probably one of measured signals)
		if (reference_signal->Get_Discrete_Levels(refTime.data(), refLevels.data(), refTime.size(), &levels_count) != S_OK)
			continue;

		// "final" resize according to real size
		refTime.resize(levels_count);
		refLevels.resize(levels_count);

		// same as in fitness calculation - if requested, do not calculate with measured levels, get approximated calculated
		if (mUseMeasuredLevels == 0)
			reference_signal->Get_Continuous_Levels(nullptr, refTime.data(), refLevels.data(), refTime.size(), glucose::apxNo_Derivation);

		if (refTime.empty())
			continue;

		auto& calcLevels = allCalcLevels[idx];
		calcLevels.resize(refTime.size());

		// repeat for new parameters 
		if (calcSignal->Get_Continuous_Levels(mTempModelParams.get(), refTime.data(), calcLevels.data(), refTime.size(), glucose::apxNo_Derivation) == S_OK)
			metric->Accumulate(refTime.data(), refLevels.data(), calcLevels.data(), refTime.size());

		calcSignals[idx] = calcSignal;
	}

	size_t tmp_size;

	// get metric value for new parameters
	double newResult;
	if (metric->Calculate(&newResult, &tmp_size, mMetricLevelsRequired) != S_OK)
		return false;

	bool hasParams = false;
	for (size_t idx = 0; idx < mClonedSegmentIds.size(); idx++)
	{
		hasParams |= (mSegments[mClonedSegmentIds[idx]].parameters != nullptr);
	}
	// everything's better than no solution
	if (!hasParams)
		return true;

	metric->Reset();

	// accumulate metric across all segments using new parameters
	for (size_t idx = 0; idx < mClonedSegmentIds.size(); idx++)
	{
		auto& calcSignal = calcSignals[idx];
		auto& refTime = allRefTimes[idx];
		auto& refLevels = allRefLevels[idx];
		auto& calcLevels = allCalcLevels[idx];

		// retrieve continuous levels in same times, and accumulate metric
		if (calcSignal->Get_Continuous_Levels(mSegments[mClonedSegmentIds[idx]].parameters.get(), refTime.data(), calcLevels.data(), refTime.size(), glucose::apxNo_Derivation) == S_OK)
			metric->Accumulate(refTime.data(), refLevels.data(), calcLevels.data(), refTime.size());
	}

	// get metric value for old parameters
	double oldResult;
	if (metric->Calculate(&oldResult, &tmp_size, mMetricLevelsRequired) != S_OK)
		return false;

	// new solution is better, when it has lower metric value
	return (newResult < oldResult);
}

bool CCompute_Holder::Is_Solve_In_Progress() const
{
	return mSolveInProgress;
}

void CCompute_Holder::Abort()
{
	// solver will abort after current iteration if this flag is set to true
	mSolverProgress.cancelled = TRUE;
}

size_t CCompute_Holder::Get_Solve_Percent_Complete() const
{
	return (size_t)round(100.0 * ((double)mSolverProgress.current_progress) / ((double)mSolverProgress.max_progress));
}

double CCompute_Holder::Get_Solve_Solution_Metric_Value() const
{
	return mSolverProgress.best_metric;
}

GUID CCompute_Holder::Get_Signal_Id() const
{
	return mSignalId;
}

glucose::SModel_Parameter_Vector CCompute_Holder::Get_Model_Parameters(uint64_t segment_id) const
{
	auto itr = mSegments.find(segment_id);
	if (itr == mSegments.end())
		return glucose::SModel_Parameter_Vector{};

	return itr->second.parameters;
}

void CCompute_Holder::Get_Improved_Segments(std::vector<uint64_t>& target) const
{
	target.clear();
	std::copy(mImprovedSegmentIds.begin(), mImprovedSegmentIds.end(), std::back_inserter(target));
}

void CCompute_Holder::Reset_Model_Parameters()
{
	for (auto& seg : mSegments)
		seg.second.parameters.reset();
}

void CCompute_Holder::Get_All_Segment_Ids(std::vector<uint64_t>& target) const
{
	target.clear();

	for (auto const& rec : mSegments)
		target.push_back(rec.first);
}
