#include "Segment_Holder.h"

CSegment_Holder::CSegment_Holder(const GUID& signalId)
	: mSignalId(signalId)
{
	//
}

void CSegment_Holder::Start_Segment(uint64_t segmentId)
{

	glucose::CTime_Segment* segment;
	if (Manufacture_Object<glucose::CTime_Segment>(&segment) != S_OK)
		return;

	mSegments[segmentId] = {
		refcnt::make_shared_reference_ext<glucose::STime_Segment, glucose::CTime_Segment>(segment, false),
		glucose::SModel_Parameter_Vector{},	// this will be set to correct value upon receiving parameters through input pipe
		true		// opened
	};
}

void CSegment_Holder::Stop_Segment(uint64_t segmentId)
{
	auto itr = mSegments.find(segmentId);
	if (itr == mSegments.end() || !itr->second.opened)
		return;

	itr->second.opened = false;
}

bool CSegment_Holder::Add_Level(uint64_t segmentId, const GUID& signal_id, double time, double level)
{
	auto itr = mSegments.find(segmentId);
	if (itr == mSegments.end() /*|| !itr->second.opened*/)
		return false;

	auto& seg = itr->second.segment;

	if (auto signal = seg.Get_Signal(signal_id))
	{
		if (signal->Add_Levels(&time, &level, 1) == S_OK)
			return true;
	}

	return false;
}


void CSegment_Holder::Set_Parameters(uint64_t segmentId, glucose::SModel_Parameter_Vector params)
{
	auto itr = mSegments.find(segmentId);
	if (itr == mSegments.end())
		return;

	itr->second.parameters = params;
}

bool CSegment_Holder::Get_Calculated_At_Time(uint64_t segmentId, const std::vector<double> &time, std::vector<double> &target)
{
	auto itr = mSegments.find(segmentId);
	if (itr == mSegments.end())	
		return false;

	auto signal = itr->second.segment.Get_Signal(mSignalId);
	if (!signal)
		return false;

	target.resize(time.size());
	if (signal->Get_Continuous_Levels(itr->second.parameters.get(), time.data(), target.data(), target.size(), glucose::apxNo_Derivation) != S_OK)
		return false;

	return true;
}

bool CSegment_Holder::Calculate_All_Values(uint64_t segmentId, std::vector<double>& times, std::vector<double>& levels)
{
	auto itr = mSegments.find(segmentId);
	if (itr == mSegments.end())
		return false;

	auto seg = itr->second.segment;
	if (!seg)
		return false;

	std::vector<glucose::SSignal> refSignals;
	const std::array<GUID, 3> refSignalGUIDs = { glucose::signal_IG, glucose::signal_BG, glucose::signal_Calibration };

	auto signal = seg.Get_Signal(mSignalId);

	for (const GUID& refGuid : refSignalGUIDs)
	{
		auto sig = seg.Get_Signal(refGuid);
		if (sig)
			refSignals.push_back(sig);
	}

	if (!signal || refSignals.empty())
		return false;

	size_t cnt;
	size_t filled;
	size_t totalCnt = 0;

	times.clear();

	for (auto refSignal : refSignals)
	{
		if (refSignal->Get_Discrete_Bounds(nullptr, &cnt) != S_OK)
			return false;

		std::vector<double> tmp_times(cnt);
		std::vector<double> tmp_levels(cnt);

		if (refSignal->Get_Discrete_Levels(tmp_times.data(), tmp_levels.data(), cnt, &filled) == S_OK)
		{
			if (filled < cnt)
			{
				cnt = filled;
				tmp_times.resize(cnt);
				tmp_levels.resize(cnt);
			}

			std::copy(tmp_times.begin(), tmp_times.end(), std::back_inserter(times));
			totalCnt += cnt;
		}
	}

	if (totalCnt == 0)
		return false;

	levels.resize(totalCnt);
	// times needs to be sorted
	std::sort(times.begin(), times.end());

	if (signal->Get_Continuous_Levels(itr->second.parameters.get(), times.data(), levels.data(), totalCnt, glucose::apxNo_Derivation) != S_OK)
		return false;

	return true;
}
