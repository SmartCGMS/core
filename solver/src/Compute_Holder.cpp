#include "Compute_Holder.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/ModelsLib.h"
#include "../../../common/rtl/SolverLib.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../factory/src/filters.h"

CCompute_Holder::CCompute_Holder(const GUID &solver_id, const GUID &signal_id, const glucose::TMetric_Parameters &metric_parameters, const size_t metric_levels_required, const char use_measured_levels,
	bool use_just_opened_segments)
	: mSolverId(solver_id), mSignalId(signal_id), mMetricParams(metric_parameters), mMetricLevelsRequired(metric_levels_required), mUseMeasuredLevels(use_measured_levels),
	  mLowBounds(nullptr), mHighBounds(nullptr), mSolveInProgress(false), mMaxTime(0.0), mLastStoppedSegment(0), mUseJustOpenedSegments(use_just_opened_segments)
{
	//
}

CCompute_Holder::~CCompute_Holder()
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
		
		bool outerBreak = false;

		
		glucose::TModel_Descriptor desc{ 0 };
		if (glucose::get_model_descriptors_by_id(mSignalId, desc)) {
			params = refcnt::Create_Container_shared<double>(desc.default_values, desc.default_values + desc.number_of_parameters);
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
	mSolutionHints.push_back(refcnt::Copy_Container<double>(parameters));		
}

void CCompute_Holder::Set_Bounds(glucose::SModel_Parameter_Vector low, glucose::SModel_Parameter_Vector high) {
	mLowBounds = low;
	mHighBounds = high;
}

void CCompute_Holder::Set_Defaults(glucose::SModel_Parameter_Vector defaults) {
	mDefaultParameters = defaults;
}

bool CCompute_Holder::Fill_Default_Model_Bounds(const GUID &signal_id, glucose::SModel_Parameter_Vector &low, glucose::SModel_Parameter_Vector &defaults, glucose::SModel_Parameter_Vector &high) {

	glucose::TModel_Descriptor desc{ 0 };
	const bool result = glucose::get_model_descriptors_by_id(signal_id, desc);
	if (result) {
		low = refcnt::Create_Container_shared<double>(desc.lower_bound, desc.lower_bound + desc.number_of_parameters);
		high = refcnt::Create_Container_shared<double>(desc.upper_bound, desc.upper_bound + desc.number_of_parameters);
		defaults = refcnt::Create_Container_shared<double>(desc.default_values, desc.default_values + desc.number_of_parameters);
	}

	return result;
}

glucose::TSolver_Setup CCompute_Holder::Prepare_Solver_Setup()
{
	// prepare metric
	glucose::SMetric metric{ mMetricParams };	

	Fill_Default_Model_Bounds(mSignalId, mLowBounds, mDefaultParameters, mHighBounds);

	mClonedSegmentIds.clear();
	// create ITime_Segment pointer to stored segments since double indirection is not polymorphic in standard way
	for (auto& seg : mSegments)
	{
		if (!seg.second.opened && mUseJustOpenedSegments)
			continue;

		if (auto sharedSegment = std::dynamic_pointer_cast<glucose::CTime_Segment>(seg.second.segment))
		{
			mClonedSegments.push_back(dynamic_cast<glucose::ITime_Segment*>(sharedSegment->Clone()));
			mClonedSegmentIds.push_back(seg.first);
		}
	}

	// if no segments are opened and we request solving, use last segment
	if (mUseJustOpenedSegments && mClonedSegments.empty() && mLastStoppedSegment != 0)
	{
		if (auto sharedSegment = std::dynamic_pointer_cast<glucose::CTime_Segment>(mSegments[mLastStoppedSegment].segment))
		{
			mClonedSegments.push_back(dynamic_cast<glucose::ITime_Segment*>(sharedSegment->Clone()));
			mClonedSegmentIds.push_back(mLastStoppedSegment);
		}
	}

	TSolution_Hint_Vector hints = mSolutionHints;
	if (hints.empty())
	{
		if (mDefaultParameters)
			hints.push_back(mDefaultParameters);
	}

	// create solver setup structure
	return {
		mSolverId,																// solver_id
		mSignalId,																// signal_id
		mClonedSegments.data(), mClonedSegments.size(),							// segments, segment_count
		metric.get(), mMetricLevelsRequired, mUseMeasuredLevels,				// metric, levels_required, use_measured_levels
		mLowBounds.get(), mHighBounds.get(),									// lower_bound, upper_bound
		mSolutionHints.data(), mSolutionHints.size(),							// solution_hints, hint_count
		mTempModelParams.get(),													// solved_parameters
		&mSolverProgress														// progress
	};
}

HRESULT CCompute_Holder::Solve(const glucose::TSolver_Setup &solverSetup)
{
	// start the solver
	HRESULT rc = solve_model_parameters(&solverSetup);

	if (rc == S_OK)
	{
		mImprovedSegmentIds.clear();

		// compare solutions using metric on current (cloned) dataset
		if (Compare_Solutions(solverSetup.metric))
		{
			for (uint64_t segId : mClonedSegmentIds)
			{
				// release old parameters
				if (mSegments[segId].parameters)
					mSegments[segId].parameters->Release();

				// replace with new parameters
				mTempModelParams->AddRef();
				mSegments[segId].parameters = mTempModelParams;
				mImprovedSegmentIds.push_back(segId);
			}
		}

		mTempModelParams->Release();

		if (mImprovedSegmentIds.empty()) // new parameters are worse for every segment
		{
			// set return code to S_FALSE --> we failed to find a better solution, so we won't propagate parameters, recalculate signal, ..
			rc = S_FALSE;
		}
	}
	else
		mTempModelParams->Release(); // this should effectivelly delete the prepared container

	mTempModelParams = nullptr;

	// release cloned segments
	for (auto seg : mClonedSegments)
		seg->Release();

	mClonedSegments.clear();
	mClonedSegmentIds.clear();	

	mSolveInProgress = false;

	return rc;
}

std::future<HRESULT> CCompute_Holder::Solve_Async()
{
	if (mSolveInProgress)
		return std::future<HRESULT>();

	mSolveInProgress = true;

	glucose::TSolver_Setup setup = Prepare_Solver_Setup();

	return std::async(std::launch::async, [&, setup]() -> HRESULT {
		return Solve(&setup);
	});
}

bool CCompute_Holder::Compare_Solutions(glucose::IMetric* metric)
{
	// no solution found - then it's not better
	if (!mTempModelParams)
		return false;

	std::vector<glucose::SSignal> calcSignals;

	// resolve matching signals from all segments
	size_t idx;
	for (idx = 0; idx < mClonedSegmentIds.size(); idx++)
	{
		glucose::STime_Segment segment = refcnt::make_shared_reference_ext<glucose::STime_Segment, glucose::ITime_Segment>(mClonedSegments[idx], true);

		calcSignals.push_back(segment.Get_Signal(mSignalId));
	}

	// all segments and their extracted times, levels and calculated levels
	std::vector<std::vector<double>> allRefTimes(mClonedSegmentIds.size());
	std::vector<std::vector<double>> allRefLevels(mClonedSegmentIds.size());
	std::vector<std::vector<double>> allCalcLevels(mClonedSegmentIds.size());

	// accumulate metric across all cloned segments using old parameters
	for (idx = 0; idx < mClonedSegmentIds.size(); idx++)
	{
		auto& calcSignal = calcSignals[idx];

		// get level count
		size_t levels_count;
		if (calcSignal->Get_Discrete_Bounds(nullptr, &levels_count) != S_OK)
			return false;

		auto& refTime = allRefTimes[idx];
		auto& refLevels = allRefLevels[idx];
		refTime.resize(levels_count);
		refLevels.resize(levels_count);

		// retrieve measured signal (if it's calculated signal, it will fall through to reference signal, which is probably one of measured signals)
		if (calcSignal->Get_Discrete_Levels(refTime.data(), refLevels.data(), refTime.size(), &levels_count) != S_OK)
			return false;

		// "final" resize according to real size
		refTime.resize(levels_count);
		refLevels.resize(levels_count);

		// same as in fitness calculation - if requested, do not calculate with measured levels, get approximated calculated
		if (mUseMeasuredLevels == 0)
			calcSignal->Get_Continuous_Levels(nullptr, refTime.data(), refLevels.data(), refTime.size(), glucose::apxNo_Derivation);

		if (refTime.empty())
			return false;

		auto& calcLevels = allCalcLevels[idx];
		calcLevels.resize(refTime.size());

		metric->Reset();

		// repeat for new parameters
		if (calcSignal->Get_Continuous_Levels(mTempModelParams, refTime.data(), calcLevels.data(), refTime.size(), glucose::apxNo_Derivation) == S_OK)
			metric->Accumulate(refTime.data(), refLevels.data(), calcLevels.data(), refTime.size());
	}

	size_t tmp_size;

	// get metric value for new parameters
	double newResult;
	if (metric->Calculate(&newResult, &tmp_size, mMetricLevelsRequired) != S_OK)
		return false;

	bool hasParams = false;
	for (idx = 0; idx < mClonedSegmentIds.size(); idx++)
	{
		hasParams |= (mSegments[mClonedSegmentIds[idx]].parameters != nullptr);
	}
	// everything's better than no solution
	if (!hasParams)
		return true;

	metric->Reset();

	// accumulate metric across all segments using new parameters
	for (idx = 0; idx < mClonedSegmentIds.size(); idx++)
	{
		auto& calcSignal = calcSignals[idx];
		auto& refTime = allRefTimes[idx];
		auto& refLevels = allRefLevels[idx];
		auto& calcLevels = allCalcLevels[idx];

		// retrieve continuous levels in same times, and accumulate metric
		if (calcSignal->Get_Continuous_Levels(mSegments[mClonedSegmentIds[idx]].parameters, refTime.data(), calcLevels.data(), refTime.size(), glucose::apxNo_Derivation) == S_OK)
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

glucose::IModel_Parameter_Vector* CCompute_Holder::Get_Model_Parameters(uint64_t segment_id) const
{
	auto itr = mSegments.find(segment_id);
	if (itr == mSegments.end())
		return nullptr;

	if (itr->second.parameters)
		itr->second.parameters->AddRef();

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
	{
		if (seg.second.parameters)
			seg.second.parameters->Release();

		seg.second.parameters = nullptr;
	}
}

void CCompute_Holder::Get_All_Segment_Ids(std::vector<uint64_t>& target) const
{
	target.clear();

	for (auto const& rec : mSegments)
		target.push_back(rec.first);
}
