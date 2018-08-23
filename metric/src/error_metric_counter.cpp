#include "error_metric_counter.h"

#include <cmath>

#undef min
#undef max


CError_Marker_Counter::CError_Marker_Counter()
{
	auto models = glucose::get_model_descriptors();
	for (const auto &model : models)
	{
		for (size_t i = 0; i < model.number_of_calculated_signals; i++)
		{
			mSignalModelParamCount[model.calculated_signal_ids[i]] = model.number_of_parameters;
			Reference_For_Calculated_Signal[model.calculated_signal_ids[i]] = model.reference_signal_ids[i];
		}
	}

	for (size_t i = 0; i < 100; i++)
	{
		mSignalModelParamCount[glucose::signal_Virtual[i]] = 6;
		Reference_For_Calculated_Signal[glucose::signal_Virtual[i]] = glucose::signal_BG;
	}

	// TODO: allow user-defined mapping for virtual signals
}

bool CError_Marker_Counter::Add_Level(uint64_t segment_id, const GUID& signal_id, double time, double level)
{
	TSegmentSignalMap& target = (Reference_For_Calculated_Signal.find(signal_id) != Reference_For_Calculated_Signal.end()) ? mCalculatedValues : mReferenceValues;

	target[segment_id][signal_id].push_back({
		time,
		level
	});

	// return true if reference level was added
	return (Reference_For_Calculated_Signal.find(signal_id) == Reference_For_Calculated_Signal.end());
}

bool CError_Marker_Counter::Recalculate_Errors()
{
	if (mCalculatedValues.empty())
		return false;

	bool result = false;
	for (auto& signal_rec : mCalculatedValues.begin()->second)
	{
		std::sort(signal_rec.second.begin(), signal_rec.second.end(), [](TValue& a, TValue& b) {
			return a.time < b.time;
		});
		result |= Recalculate_Errors_For(signal_rec.first);
	}

	return result;
}

void CError_Marker_Counter::Reset_Segment(uint64_t segment_id, const GUID& signal_id)
{
	// reset calculated values only
	if (Reference_For_Calculated_Signal.find(signal_id) != Reference_For_Calculated_Signal.end())
		mCalculatedValues[segment_id][signal_id].clear();
}

void CError_Marker_Counter::Reset_All()
{
	mCalculatedValues.clear();
	mReferenceValues.clear();
}

HRESULT CError_Marker_Counter::Get_Errors(const GUID &signal_id, const glucose::NError_Type type, glucose::TError_Markers &markers) {
	if (mErrors.find(signal_id) == mErrors.end())
		return ENOENT;
	
	markers = mErrors[signal_id][static_cast<size_t>(type)];

	return S_OK;
}

static void Get_Quantiles(const std::vector<double>& data, const std::vector<double>& percents, double* const target)
{
	if (data.size() < 2)
		return;

	for (size_t i = 0; i < percents.size(); i++)
	{
		const double pos = percents[i] * data.size() - 0.5;

		const size_t left = std::max((size_t)floor(pos), (size_t)0);
		const size_t right = std::min((size_t)ceil(pos), data.size() - 1);

		target[i] = data[left] + left*(data[left] - data[right]) - pos*(data[left] - data[right]);
	}
}

static double Get_Standard_Deviation(const std::vector<double>& data, const double& mean)
{
	double accumulator = 0;
	for (size_t i = 0; i < data.size(); i++)
	{
		const double err = data[i] - mean;
		accumulator += err * err;
	}

	double correction_count = static_cast<double>(data.size());
	if (correction_count > 1.5) correction_count -= 1.5;
		else if (correction_count > 1.0) correction_count -= 1.0;	//if not, try to fall back to Bessel's Correction at least


	return sqrt(accumulator / correction_count);
}

static double Get_AIC(const std::vector<double>& abs_diffs, size_t parameter_count) {

	if (abs_diffs.empty()) return std::numeric_limits<double>::quiet_NaN();

	double RSSum = 0.0;
	for (const double &diff : abs_diffs) 
		RSSum += diff * diff;
	
	
	const double dbl_size = static_cast<double>(abs_diffs.size());
	
	return dbl_size * (log(RSSum) / dbl_size) + 2.0*static_cast<double>(parameter_count);


//	double cnt = static_cast<double>(parameter_count);
//	return 2.0*cnt - 2.0*log(mean);
}

bool CError_Marker_Counter::Recalculate_Errors_For(const GUID& signal_id)
{
	auto const itr = Reference_For_Calculated_Signal.find(signal_id);
	if (itr == Reference_For_Calculated_Signal.end())
		return false;

	const GUID& ref_signal_id = itr->second;

	std::vector<double> refValues, calcValues;
	for (auto const& segRef : mReferenceValues)
	{
		auto const& refSigItr = segRef.second.find(ref_signal_id);
		if (refSigItr == segRef.second.end() || refSigItr->second.empty())
			continue;

		auto const& calcSigItr = mCalculatedValues[segRef.first].find(signal_id);
		if (calcSigItr == mCalculatedValues[segRef.first].end() || calcSigItr->second.empty())
			continue;

		auto const& refVec = refSigItr->second;
		auto const& calcVec = calcSigItr->second;

		size_t refIdx = 0, calcIdx = 0;

		// find first reference value "inside" calculated ones
		while (refIdx < refVec.size() && refVec[refIdx].time < calcVec[0].time)
			refIdx++;

		if (refIdx == refVec.size())
			continue;

		for (; refIdx < refVec.size(); refIdx++)
		{
			while (calcIdx < calcVec.size() && calcVec[calcIdx].time < refVec[refIdx].time)
				calcIdx++;

			if (calcIdx == calcVec.size())
				break;

			refValues.push_back(refVec[refIdx].value);

			if ((calcIdx>0) && (fabs(calcVec[calcIdx - 1].time - refVec[refIdx].time) < fabs(calcVec[calcIdx].time - refVec[refIdx].time)))
				calcValues.push_back(calcVec[calcIdx - 1].value);
			else
				calcValues.push_back(calcVec[calcIdx].value);
		}
	}

	const size_t cnt = refValues.size();

	glucose::TError_Markers& absErr = mErrors[signal_id][(size_t)glucose::NError_Type::Absolute];
	glucose::TError_Markers& relErr = mErrors[signal_id][(size_t)glucose::NError_Type::Relative];

	if (cnt == 0)
	{
		memset(&absErr, 0, sizeof(absErr));
		memset(&relErr, 0, sizeof(relErr));
		return false;
	}

	std::vector<double> absolute(cnt);
	std::vector<double> relative(cnt);
	double absoluteAvg = 0.0;
	double relativeAvg = 0.0;

	// calculate absolute and relative errors
	for (size_t i = 0; i < cnt; i++)
	{
		const double delta = fabs(refValues[i] - calcValues[i]);

		absolute[i] = delta;
		relative[i] += delta / refValues[i];
	}

	// sort values in ascending order
	std::sort(absolute.begin(), absolute.end(), std::less<double>());
	// sum all absolute errors and calculate average
	absoluteAvg = std::accumulate(absolute.begin(), absolute.end(), 0.0) / static_cast<double>(cnt);

	// same for relative error
	std::sort(relative.begin(), relative.end(), std::less<double>());
	relativeAvg = std::accumulate(relative.begin(), relative.end(), 0.0) / static_cast<double>(cnt);

	// fill absolute error container
	absErr.avg = absoluteAvg;
	Get_Quantiles(absolute, { 0.25, 0.5, 0.75, 0.95, 0.99 }, &absErr.percentile[1] );
	absErr.minval = absolute[0];
	absErr.maxval = absolute[cnt - 1];
	absErr.stddev = Get_Standard_Deviation(absolute, absoluteAvg);
	absErr.aic = Get_AIC(absolute, (mSignalModelParamCount.find(signal_id) == mSignalModelParamCount.end()) ? 1 : mSignalModelParamCount[signal_id]);

	// fill relative error container
	relErr.avg = relativeAvg;
	Get_Quantiles(relative, { 0.25, 0.5, 0.75, 0.95, 0.99 }, &relErr.percentile[1]);
	relErr.minval = relative[0];
	relErr.maxval = relative[cnt - 1];
	relErr.stddev = Get_Standard_Deviation(relative, relativeAvg);
	relErr.aic = std::numeric_limits<double>::quiet_NaN();//Get_AIC(relative, relativeAvg);

	return true;
}
