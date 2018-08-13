#include "drawing.h"

#include "../../../common/lang/dstrings.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/rattime.h"

#include "drawing/Generators/IGenerator.h"
#include "drawing/Generators/GraphGenerator.h"
#include "drawing/Generators/ClarkGenerator.h"
#include "drawing/Generators/ParkesGenerator.h"
#include "drawing/Generators/AGPGenerator.h"
#include "drawing/Generators/DayGenerator.h"
#include "drawing/Generators/ECDFGenerator.h"

#include <map>
#include <tbb/tbb_allocator.h>

#include <iostream>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <set>

#undef min

const std::map<GUID, const char*, std::less<GUID>, tbb::tbb_allocator<std::pair<const GUID, const char*>>> Signal_Mapping = {
	{ glucose::signal_IG, "ist" },
	{ glucose::signal_BG, "blood" },
	{ glucose::signal_Calibration, "bloodCalibration" },
	{ glucose::signal_ISIG, "isig" },
	{ glucose::signal_Insulin, "insulin" },
	{ glucose::signal_Carb_Intake, "carbs" },
};

CDrawing_Filter::CDrawing_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{ inpipe }, mOutput{ outpipe }, mGraphMaxValue(-1) {}

HRESULT IfaceCalling CDrawing_Filter::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<glucose::IFilter>(glucose::Drawing_Filter, *riid, ppvObj)) return S_OK;
	if (Internal_Query_Interface<glucose::IDrawing_Filter_Inspection>(glucose::Drawing_Filter_Inspection, *riid, ppvObj)) return S_OK;

	return E_NOINTERFACE;
}

void CDrawing_Filter::Run_Main() {
	std::set<GUID> signalsBeingReset;

	mInputData.clear();

	// TODO: get rid of excessive locking (mutexes)

	for (;  glucose::UDevice_Event evt = mInput.Receive(); ) {

		// explicit "changed lock" scope; unblocked right before pipe operation to avoid deadlock
		{
			std::unique_lock<std::mutex> lck(mChangedMtx);

			// incoming level or calibration - store to appropriate vector
			if (evt.event_code == glucose::NDevice_Event_Code::Level )
			{
				//std::unique_lock<std::mutex> lck(mChangedMtx);

				mInputData[evt.signal_id][evt.segment_id].push_back(Value(evt.level, Rat_Time_To_Unix_Time(evt.device_time), evt.segment_id));
				// signal is not being reset (recalculated and resent) right now - set changed flag
				if (signalsBeingReset.find(evt.signal_id) == signalsBeingReset.end())
					mChanged = true;

				// several signals are excluded from maximum value determining, since their values are not in mmol/l, and are drawn in a different way
				// TODO: more generic way to determine value units
				if (evt.signal_id != glucose::signal_Carb_Intake && evt.signal_id != glucose::signal_Insulin && evt.signal_id != glucose::signal_Health_Stress)
				{
					if (evt.level > mGraphMaxValue)
						mGraphMaxValue = std::min(evt.level, 150.0); //http://www.guinnessworldrecords.com/world-records/highest-blood-sugar-level/
				}
			}
			// incoming new parameters
			else if (evt.event_code == glucose::NDevice_Event_Code::Parameters)
			{
				mParameterChanges[evt.signal_id][evt.segment_id].push_back(Value((double)Rat_Time_To_Unix_Time(evt.device_time), Rat_Time_To_Unix_Time(evt.device_time), evt.segment_id));
			}
			// incoming parameters reset information message
			else if (evt.event_code == glucose::NDevice_Event_Code::Information)
			{
				// we catch parameter reset information message
				if (evt.info == rsParameters_Reset)
				{
					// TODO: verify, if parameter reset came for signal, that is really a calculated signal (not measured)

					// reset of specific signal
					if (evt.signal_id != Invalid_GUID)
					{
						Reset_Signal(evt.signal_id, evt.segment_id);
						signalsBeingReset.insert(evt.signal_id);
					}
					else // reset of all calculated signals (signal == Invalid_GUID)
					{
						for (auto& data : mInputData)
						{
							if (Signal_Mapping.find(data.first) == Signal_Mapping.end())
							{
								Reset_Signal(data.first, evt.segment_id);
								signalsBeingReset.insert(evt.signal_id);
							}
						}
					}
				}
				else if (evt.info == rsSegment_Recalculate_Complete)
				{
					signalsBeingReset.erase(evt.signal_id);
					mChanged = true;
				}
			}
			else if (evt.event_code == glucose::NDevice_Event_Code::Time_Segment_Start || evt.event_code == glucose::NDevice_Event_Code::Time_Segment_Stop)
			{
				mSegmentMarkers.push_back(Value(0, Rat_Time_To_Unix_Time(evt.device_time), evt.segment_id));
			}
		}

		if (!mOutput.Send(evt))
			break;
	}
}

bool CDrawing_Filter::Redraw_If_Changed(const std::unordered_set<uint64_t> &segmentIds, const std::set<GUID> &signalIds)
{
	std::unique_lock<std::mutex> lck(mChangedMtx);

	if (mChanged || !segmentIds.empty() || !signalIds.empty())
	{
		mChanged = false;

		mDataMap.clear();
		Prepare_Drawing_Map(segmentIds, signalIds);
		// we don't need that lock anymore
		lck.unlock();

		Generate_Graphs(mDataMap, mGraphMaxValue, mLocaleMap);

		return true;
	}

	return false;
}

void CDrawing_Filter::Reset_Signal(const GUID& signal_id, const uint64_t segment_id)
{
	// incoming parameters just erases the signals, because a new set of calculated levels comes through the pipe
	// remove just segment, for which the reset was issued
	for (auto& dataPair : mInputData[signal_id])
	{
		auto& data = dataPair.second;
		data.erase(
			std::remove_if(data.begin(), data.end(), [segment_id](Value const& a) { return a.segment_id == segment_id; }),
			data.end()
		);
	}
	// remove also markers
	for (auto& dataPair : mInputData[signal_id])
	{
		auto& data = dataPair.second;
		data.erase(
			std::remove_if(data.begin(), data.end(), [segment_id](Value const& a) { return a.segment_id == segment_id; }),
			data.end()
		);
	}
}

void CDrawing_Filter::Prepare_Drawing_Map(const std::unordered_set<uint64_t> &segmentIds, const std::set<GUID> &signalIds) {

	const bool allSegments = (segmentIds.empty());
	const bool allSignals = (signalIds.empty());

	DataMap vectorsMap;

	for (auto const& mapping : Signal_Mapping)
	{
		// do not use signals we don't want to draw
		if (!allSignals && signalIds.find(mapping.first) == signalIds.end())
			continue;

		for (auto& dataPair : mInputData[mapping.first])
		{
			auto& data = dataPair.second;
			std::sort(data.begin(), data.end(), [](Value& a, Value& b) {
				return a.date < b.date;
			});
		}

		vectorsMap[mapping.second] = Data({}, true, mInputData[mapping.first].empty(), false);

		for (auto& dataPair : mInputData[mapping.first])
		{
			if (allSegments || segmentIds.find(dataPair.first) != segmentIds.end())
			{
				const auto& data = dataPair.second;
				vectorsMap[mapping.second].values.reserve(vectorsMap[mapping.second].values.size() + data.size());
				std::copy(data.begin(), data.end(), std::back_inserter(vectorsMap[mapping.second].values));
			}
		}
	}

	vectorsMap["segment_markers"] = Data(mSegmentMarkers, true, mSegmentMarkers.empty(), false);

	std::string base = "calc";
	size_t curIdx = 0;

	for (auto const& presentData : mInputData)
	{
		// do not use signals we don't want to draw
		if (!allSignals && signalIds.find(presentData.first) == signalIds.end())
			continue;

		if (Signal_Mapping.find(presentData.first) == Signal_Mapping.end())
		{
			const GUID& signal_id = presentData.first;

			std::string key = base + std::to_string(curIdx);

			for (auto& dataPair : mInputData[signal_id])
			{
				auto& data = dataPair.second;
				std::sort(data.begin(), data.end(), [](Value& a, Value& b) {
					return a.date < b.date;
				});
			}

			vectorsMap[key] = Data({}, true, false, true);
			for (auto& dataPair : mInputData[signal_id])
			{
				if (allSegments || segmentIds.find(dataPair.first) != segmentIds.end())
				{
					const auto& data = dataPair.second;
					vectorsMap[key].values.reserve(vectorsMap[key].values.size() + data.size());
					std::copy(data.begin(), data.end(), std::back_inserter(vectorsMap[key].values));
				}
			}

			vectorsMap[key].identifier = key;

			if (mParameterChanges.find(signal_id) != mParameterChanges.end())
			{
				const std::string pkey = "param_" + key;
				vectorsMap[pkey] = Data({}, true, false, false);
				for (auto& dataPair : mParameterChanges[signal_id])
				{
					if (allSegments || segmentIds.find(dataPair.first) != segmentIds.end())
					{
						const auto& data = dataPair.second;
						vectorsMap[pkey].values.reserve(vectorsMap[pkey].values.size() + data.size());
						std::copy(data.begin(), data.end(), std::back_inserter(vectorsMap[pkey].values));
					}
				}

				vectorsMap[pkey].identifier = pkey;
			}

			auto itr = mCalcSignalNameMap.find(signal_id);
			if (itr != mCalcSignalNameMap.end())
				mLocaleMap[key] = itr->second;
			else
				mLocaleMap[key] = "???";

			curIdx++;
		}
	}

	mDataMap = vectorsMap;
}

HRESULT CDrawing_Filter::Run(glucose::IFilter_Configuration* const configuration) {
	mCanvasWidth = 0;
	mCanvasHeight = 0;

	auto conf = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);

	mCanvasWidth = static_cast<int>(conf.Read_Int(rsDrawing_Filter_Canvas_Width));
	mCanvasHeight = static_cast<int>(conf.Read_Int(rsDrawing_Filter_Canvas_Height));
	mAGP_FilePath = conf.Read_String(rsDrawing_Filter_Filename_AGP);
	mClark_FilePath = conf.Read_String(rsDrawing_Filter_Filename_Clark);
	mDay_FilePath = conf.Read_String(rsDrawing_Filter_Filename_Day);
	mGraph_FilePath = conf.Read_String(rsDrawing_Filter_Filename_Graph);
	mParkes_FilePath = conf.Read_String(rsDrawing_Filter_Filename_Parkes);
	mECDF_FilePath = conf.Read_String(rsDrawing_Filter_Filename_ECDF);

	if (mCanvasWidth != 0 && mCanvasHeight != 0)
	{
		CAGP_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CClark_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CDay_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CGraph_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CParkes_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CECDF_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
	}

	// cache model signal names
	auto models = glucose::get_model_descriptors();
	for (auto const& model : models)
	{
		for (size_t i = 0; i < model.number_of_calculated_signals; i++)
		{
			std::wstring wname = std::wstring(model.description) + L" - " + model.calculated_signal_names[i];
			mCalcSignalNameMap[model.calculated_signal_ids[i]] = std::string{wname.begin(), wname.end()};
		}
	}

	for (size_t i = 0; i < glucose::signal_Virtual.size(); i++)
		mCalcSignalNameMap[glucose::signal_Virtual[i]] = std::string("virtual ") + std::to_string(i);

	Fill_Localization_Map(mLocaleMap);

	Run_Main();

	// when the filter shuts down, store drawings to files
	Redraw_If_Changed();

	Store_To_File(mGraph_SVG, mGraph_FilePath);
	Store_To_File(mClark_SVG, mClark_FilePath);
	Store_To_File(mDay_SVG, mDay_FilePath);
	Store_To_File(mAGP_SVG, mAGP_FilePath);
	Store_To_File(mParkes_type1_SVG, mParkes_FilePath);
	Store_To_File(mECDF_SVG, mECDF_FilePath);

	return S_OK;
}

inline std::string WToMB(std::wstring in)
{
	return std::string(in.begin(), in.end());
}

void CDrawing_Filter::Store_To_File(std::string& str, std::wstring& filePath)
{
	if (filePath.empty())
		return;

	std::string sbase(filePath.begin(), filePath.end());

	std::ofstream ofs(sbase);
	ofs << str;
}

void CDrawing_Filter::Generate_Graphs(DataMap& valueMap, double maxValue, LocalizationMap& locales)
{
	// lock scope
	{
		std::unique_lock<std::mutex> lck(mRetrieveMtx);

		// graph generator scope
		{
			Set_Locale_Title(locales, dsDrawingLocaleTitle);
			CGraph_Generator graph(valueMap, maxValue, locales, 1);
			mGraph_SVG = graph.Build_SVG();
		}

		// clark grid generator scope
		{
			Set_Locale_Title(locales, dsDrawingLocaleTitleClark);
			CClark_Generator graph(valueMap, maxValue, locales, 1);
			mClark_SVG = graph.Build_SVG();
		}

		// day plot generator scope
		{
			Set_Locale_Title(locales, dsDrawingLocaleTitleDay);
			CDay_Generator graph(valueMap, maxValue, locales, 1);
			mDay_SVG = graph.Build_SVG();
		}

		// AGP generator scope
		{
			Set_Locale_Title(locales, dsDrawingLocaleTitleAgp);
			CAGP_Generator graph(valueMap, maxValue, locales, 1);
			mAGP_SVG = graph.Build_SVG();
		}

		// parkes grid generator scope (type 1)
		{
			Set_Locale_Title(locales, dsDrawingLocaleTitleParkes);
			CParkes_Generator graph(valueMap, maxValue, locales, 1, true);
			mParkes_type1_SVG = graph.Build_SVG();
		}

		// parkes grid generator scope (type 2)
		{
			Set_Locale_Title(locales, dsDrawingLocaleTitleParkes);
			CParkes_Generator graph(valueMap, maxValue, locales, 1, false);
			mParkes_type2_SVG = graph.Build_SVG();
		}

		// ECDF generator scope
		{
			Set_Locale_Title(locales, dsDrawingLocaleTitleParkes);
			CECDF_Generator graph(valueMap, maxValue, locales, 1);
			mECDF_SVG = graph.Build_SVG();
		}
	}
}

HRESULT CDrawing_Filter::Get_Plot(const std::string &plot, refcnt::IVector_Container<char> *svg) const {
	char* plot_ptr = const_cast<char*>(plot.data());
	return svg->set(plot_ptr, plot_ptr + plot.size());
}

HRESULT IfaceCalling CDrawing_Filter::Draw(glucose::TDrawing_Image_Type type, glucose::TDiagnosis diagnosis, refcnt::str_container *svg, refcnt::IVector_Container<uint64_t> *segmentIds, refcnt::IVector_Container<GUID> *signalIds) {

	std::unordered_set<uint64_t> segmentSet{};
	std::set<GUID> signalSet{};

	// draw only selected segments
	if (segmentIds != nullptr)
	{
		auto vec = refcnt::Container_To_Vector(segmentIds);
		segmentSet.insert(vec.begin(), vec.end());
	}
	// draw only selected signals
	if (signalIds != nullptr)
	{
		auto vec = refcnt::Container_To_Vector(signalIds);
		signalSet.insert(vec.begin(), vec.end());
	}

	Redraw_If_Changed(segmentSet, signalSet);

	std::unique_lock<std::mutex> lck(mRetrieveMtx);

	switch (type)
	{
		case glucose::TDrawing_Image_Type::AGP:
			return Get_Plot(mAGP_SVG, svg);
		case glucose::TDrawing_Image_Type::Day:
			return Get_Plot(mDay_SVG, svg);
		case glucose::TDrawing_Image_Type::Graph:
			return Get_Plot(mGraph_SVG, svg);
		case glucose::TDrawing_Image_Type::Clark:
			return Get_Plot(mClark_SVG, svg);
		case glucose::TDrawing_Image_Type::Parkes:
		{
			switch (diagnosis)
			{
				case glucose::TDiagnosis::Type1:
					return Get_Plot(mParkes_type1_SVG, svg);
				case glucose::TDiagnosis::Type2:
				case glucose::TDiagnosis::Gestational:
					return Get_Plot(mParkes_type2_SVG, svg);
					// TODO: Parkes error grid for gestational diabetes
				default:
					break;
			}
		}
		case glucose::TDrawing_Image_Type::ECDF:
			return Get_Plot(mECDF_SVG, svg);
		default:
			break;
	}

	return E_INVALIDARG;
}

void CDrawing_Filter::Set_Locale_Title(LocalizationMap& locales, std::wstring title) const
{
	locales[WToMB(rsDrawingLocaleTitle)] = WToMB(title);
}

void CDrawing_Filter::Fill_Localization_Map(LocalizationMap& locales)
{
	locales[WToMB(rsDrawingLocaleTitle)] = WToMB(dsDrawingLocaleTitle);
	locales[WToMB(rsDrawingLocaleTitleDay)] = WToMB(dsDrawingLocaleTitleDay);
	locales[WToMB(rsDrawingLocaleSubtitle)] = WToMB(dsDrawingLocaleSubtitle);
	locales[WToMB(rsDrawingLocaleDiabetes1)] = WToMB(dsDrawingLocaleDiabetes1);
	locales[WToMB(rsDrawingLocaleDiabetes2)] = WToMB(dsDrawingLocaleDiabetes2);
	locales[WToMB(rsDrawingLocaleTime)] = WToMB(dsDrawingLocaleTime);
	locales[WToMB(rsDrawingLocaleTimeDay)] = WToMB(dsDrawingLocaleTimeDay);
	locales[WToMB(rsDrawingLocaleConcentration)] = WToMB(dsDrawingLocaleConcentration);
	locales[WToMB(rsDrawingLocaleHypoglycemy)] = WToMB(dsDrawingLocaleHypoglycemy);
	locales[WToMB(rsDrawingLocaleHyperglycemy)] = WToMB(dsDrawingLocaleHyperglycemy);
	locales[WToMB(rsDrawingLocaleBlood)] = WToMB(dsDrawingLocaleBlood);
	locales[WToMB(rsDrawingLocaleBloodCalibration)] = WToMB(dsDrawingLocaleBloodCalibration);
	locales[WToMB(rsDrawingLocaleIst)] = WToMB(dsDrawingLocaleIst);
	locales[WToMB(rsDrawingLocaleResults)] = WToMB(dsDrawingLocaleResults);
	locales[WToMB(rsDrawingLocaleDiff2)] = WToMB(dsDrawingLocaleDiff2);
	locales[WToMB(rsDrawingLocaleDiff3)] = WToMB(dsDrawingLocaleDiff3);
	locales[WToMB(rsDrawingLocaleQuantile)] = WToMB(dsDrawingLocaleQuantile);
	locales[WToMB(rsDrawingLocaleRelative)] = WToMB(dsDrawingLocaleRelative);
	locales[WToMB(rsDrawingLocaleAbsolute)] = WToMB(dsDrawingLocaleAbsolute);
	locales[WToMB(rsDrawingLocaleAverage)] = WToMB(dsDrawingLocaleAverage);
	locales[WToMB(rsDrawingLocaleType)] = WToMB(dsDrawingLocaleType);
	locales[WToMB(rsDrawingLocaleError)] = WToMB(dsDrawingLocaleError);
	locales[WToMB(rsDrawingLocaleDescription)] = WToMB(dsDrawingLocaleDescription);
	locales[WToMB(rsDrawingLocaleColor)] = WToMB(dsDrawingLocaleColor);
	locales[WToMB(rsDrawingLocaleCounted)] = WToMB(dsDrawingLocaleCounted);
	locales[WToMB(rsDrawingLocaleMeasured)] = WToMB(dsDrawingLocaleMeasured);
	locales[WToMB(rsDrawingLocaleAxisX)] = WToMB(dsDrawingLocaleAxisX);
	locales[WToMB(rsDrawingLocaleAxisY)] = WToMB(dsDrawingLocaleAxisY);
	locales[WToMB(rsDrawingLocaleSvgDatetimeTitle)] = WToMB(dsDrawingLocaleSvgDatetimeTitle);
	locales[WToMB(rsDrawingLocaleSvgIstTitle)] = WToMB(dsDrawingLocaleSvgIstTitle);
	locales[WToMB(rsDrawingLocaleSvgBloodTitle)] = WToMB(dsDrawingLocaleSvgBloodTitle);
	locales[WToMB(rsDrawingLocaleSvgBloodCalibrationTitle)] = WToMB(dsDrawingLocaleSvgBloodCalibrationTitle);
	locales[WToMB(rsDrawingLocaleSvgInsulinTitle)] = WToMB(dsDrawingLocaleSvgInsulinTitle);
	locales[WToMB(rsDrawingLocaleSvgCarbohydratesTitle)] = WToMB(dsDrawingLocaleSvgCarbohydratesTitle);
	locales[WToMB(rsDrawingLocaleSvgISIGTitle)] = WToMB(dsDrawingLocaleSvgISIGTitle);
	locales[WToMB(rsDrawingLocaleSvgDiff2Title)] = WToMB(dsDrawingLocaleSvgDiff2Title);
	locales[WToMB(rsDrawingLocaleSvgDiff3Title)] = WToMB(dsDrawingLocaleSvgDiff3Title);
	locales[WToMB(rsDrawingLocaleRelativeError)] = WToMB(dsDrawingLocaleRelativeError);
	locales[WToMB(rsDrawingLocaleCummulativeProbability)] = WToMB(dsDrawingLocaleCummulativeProbability);
	locales[WToMB(rsDrawingLocaleElevatedGlucose)] = WToMB(dsDrawingLocaleElevatedGlucose);
}
