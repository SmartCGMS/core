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

#include <iostream>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <set>

#undef min

const std::map<GUID, const char*> Signal_Mapping = {
	{ glucose::signal_IG, "ist" },
	{ glucose::signal_BG, "blood" },
	{ glucose::signal_Calibration, "bloodCalibration" },
	{ glucose::signal_ISIG, "isig" },
	{ glucose::signal_Insulin, "insulin" },
	{ glucose::signal_Carb_Intake, "carbs" },
};

CDrawing_Filter::CDrawing_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{ inpipe }, mOutput{ outpipe }, mDiagnosis(1), mRedrawPeriod(500), mGraphMaxValue(-1) {}

HRESULT IfaceCalling CDrawing_Filter::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<glucose::IFilter>(glucose::Drawing_Filter, *riid, ppvObj)) return S_OK;
	if (Internal_Query_Interface<glucose::IDrawing_Filter_Inspection>(glucose::Drawing_Filter_Inspection, *riid, ppvObj)) return S_OK;

	return E_NOINTERFACE;
}

void CDrawing_Filter::Run_Main() {
	std::set<GUID> signalsBeingReset;

	mInputData.clear();

	// TODO: get rid of excessive locking (mutexes)

	for (;  glucose::UDevice_Event evt = mInput.Receive(); evt) {

		// explicit "changed lock" scope; unblocked right before pipe operation to avoid deadlock
		{
			std::unique_lock<std::mutex> lck(mChangedMtx);

			// incoming level or calibration - store to appropriate vector
			if (evt.event_code == glucose::NDevice_Event_Code::Level || evt.event_code == glucose::NDevice_Event_Code::Calibrated)
			{
				//std::unique_lock<std::mutex> lck(mChangedMtx);

				mInputData[evt.signal_id].push_back(Value(evt.level, Rat_Time_To_Unix_Time(evt.device_time), evt.segment_id));
				// signal is not being reset (recalculated and resent) right now - set changed flag
				if (signalsBeingReset.find(evt.signal_id) == signalsBeingReset.end())
					mChanged = true;

				// for now, update maximum value just from these signals
				//if (evt.signal_id == glucose::signal_IG || evt.signal_id == glucose::signal_BG)
				{
					if (evt.level > mGraphMaxValue)
						mGraphMaxValue = std::min(evt.level, 150.0); //http://www.guinnessworldrecords.com/world-records/highest-blood-sugar-level/
				}
			}
			// incoming new parameters
			else if (evt.event_code == glucose::NDevice_Event_Code::Parameters)
			{
				mParameterChanges[evt.signal_id].push_back(Value((double)Rat_Time_To_Unix_Time(evt.device_time), Rat_Time_To_Unix_Time(evt.device_time), evt.segment_id));
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

	// set running flag to false and notify scheduler thread
	mRunning = false;

	// lock scope
	{
		std::unique_lock<std::mutex> lck(mChangedMtx);
		mSchedCv.notify_all();
	}

	// join until the thread ends
	if (mSchedulerThread->joinable())
		mSchedulerThread->join();
}

void CDrawing_Filter::Run_Scheduler()
{
	Fill_Localization_Map(mLocaleMap);

	while (mRunning)
	{
		std::unique_lock<std::mutex> lck(mChangedMtx);

		if (mChanged)
		{
			mChanged = false;

			mDataMap.clear();
			Prepare_Drawing_Map();
			Generate_Graphs(mDataMap, mGraphMaxValue, mLocaleMap);
		}

		mSchedCv.wait_for(lck, std::chrono::milliseconds(mRedrawPeriod));
	}
}

void CDrawing_Filter::Reset_Signal(const GUID& signal_id, const uint64_t segment_id)
{
	// incoming parameters just erases the signals, because a new set of calculated levels comes through the pipe
	// remove just segment, for which the reset was issued
	mInputData[signal_id].erase(
		std::remove_if(mInputData[signal_id].begin(), mInputData[signal_id].end(), [segment_id](Value const& a) { return a.segment_id == segment_id; }),
		mInputData[signal_id].end()
	);
	// remove also markers
	mParameterChanges[signal_id].erase(
		std::remove_if(mParameterChanges[signal_id].begin(), mParameterChanges[signal_id].end(), [segment_id](Value const& a) { return a.segment_id == segment_id; }),
		mParameterChanges[signal_id].end()
	);
}

void CDrawing_Filter::Prepare_Drawing_Map() {

	DataMap vectorsMap;

	for (auto const& mapping : Signal_Mapping)
	{
		std::sort(mInputData[mapping.first].begin(), mInputData[mapping.first].end(), [](Value& a, Value& b) {
			return a.date < b.date;
		});
		vectorsMap[mapping.second] = Data(mInputData[mapping.first], true, mInputData[mapping.first].empty(), false);
	}

	vectorsMap["segment_markers"] = Data(mSegmentMarkers, true, mSegmentMarkers.empty(), false);

	std::string base = "calc";
	size_t curIdx = 0;

	for (auto const& presentData : mInputData)
	{
		if (Signal_Mapping.find(presentData.first) == Signal_Mapping.end())
		{
			const GUID& signal_id = presentData.first;

			std::string key = base + std::to_string(curIdx);

			std::sort(mInputData[signal_id].begin(), mInputData[signal_id].end(), [](Value& a, Value& b) {
				return a.date < b.date;
			});

			vectorsMap[key] = Data(mInputData[signal_id], true, false, true);
			vectorsMap[key].identifier = key;

			if (mParameterChanges.find(signal_id) != mParameterChanges.end())
			{
				const std::string pkey = "param_" + key;
				vectorsMap[pkey] = Data(mParameterChanges[signal_id], true, false, false);
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

	wchar_t *begin, *end;

	// lambda for common code (convert wstr_container to wstring)
	auto wstrConv = [&begin, &end](glucose::TFilter_Parameter const* param) -> std::wstring {

		if (param->wstr->get(&begin, &end) != S_OK)
			return std::wstring();

		return std::wstring(begin, end);
	};

	glucose::TFilter_Parameter *cbegin, *cend;
	if ((configuration != nullptr) && (configuration->get(&cbegin, &cend) == S_OK)) {
		for (glucose::TFilter_Parameter* cur = cbegin; cur < cend; cur += 1)
		{
			if (cur->config_name->get(&begin, &end) != S_OK)
				continue;

			std::wstring confname{ begin, end };

			if (confname == rsDrawing_Filter_Period)
				mRedrawPeriod = cur->int64;
			else if (confname == rsDiagnosis_Is_Type2)
				mDiagnosis = cur->boolean ? 2 : 1;
			else if (confname == rsDrawing_Filter_Canvas_Width)
				mCanvasWidth = static_cast<int>(cur->int64);
			else if (confname == rsDrawing_Filter_Canvas_Height)
				mCanvasHeight = static_cast<int>(cur->int64);
			else if (confname == rsDrawing_Filter_Filename_AGP)
				mAGP_FilePath = wstrConv(cur);
			else if (confname == rsDrawing_Filter_Filename_Clark)
				mClark_FilePath = wstrConv(cur);
			else if (confname == rsDrawing_Filter_Filename_Day)
				mDay_FilePath = wstrConv(cur);
			else if (confname == rsDrawing_Filter_Filename_Graph)
				mGraph_FilePath = wstrConv(cur);
			else if (confname == rsDrawing_Filter_Filename_Parkes)
				mParkes_FilePath = wstrConv(cur);
			else if (confname == rsDrawing_Filter_Filename_ECDF)
				mECDF_FilePath = wstrConv(cur);
		}
	}

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

	mRunning = true;

	mSchedulerThread = std::make_unique<std::thread>(&CDrawing_Filter::Run_Scheduler, this);

	Run_Main();

	return S_OK;
}

inline std::string WToMB(std::wstring in)
{
	return std::string(in.begin(), in.end());
}

void CDrawing_Filter::Store_To_File(std::string& str, std::wstring& filePath)
{
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

	if (!mGraph_FilePath.empty())
		Store_To_File(mGraph_SVG, mGraph_FilePath);
	if (!mClark_FilePath.empty())
		Store_To_File(mClark_SVG, mClark_FilePath);
	if (!mDay_FilePath.empty())
		Store_To_File(mDay_SVG, mDay_FilePath);
	if (!mAGP_FilePath.empty())
		Store_To_File(mAGP_SVG, mAGP_FilePath);
	if (!mParkes_FilePath.empty())
		Store_To_File(mParkes_type1_SVG, mParkes_FilePath);
	if (!mECDF_FilePath.empty())
		Store_To_File(mECDF_SVG, mECDF_FilePath);

	glucose::UDevice_Event drawEvt{ glucose::NDevice_Event_Code::Information };	
	drawEvt.info.set(rsInfo_Redraw_Complete);
	mOutput.Send(drawEvt);
}

HRESULT CDrawing_Filter::Get_Plot(const std::string &plot, refcnt::IVector_Container<char> *svg) const {
	return svg->set(plot.data(), plot.data() + plot.size());
}

HRESULT IfaceCalling CDrawing_Filter::Draw(glucose::TDrawing_Image_Type type, glucose::TDiagnosis diagnosis, refcnt::str_container *svg) const
{
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
