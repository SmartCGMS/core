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

#include <iostream>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <set>

std::atomic<CDrawing_Filter*> CDrawing_Filter::mInstance = nullptr;

CDrawing_Filter::CDrawing_Filter(glucose::IFilter_Pipe* inpipe, glucose::IFilter_Pipe* outpipe)
	: mInput(inpipe), mOutput(outpipe), mRedrawPeriod(500), mGraphMaxValue(-1), mDiagnosis(1)
{
	//
}

void CDrawing_Filter::Run_Main()
{
	glucose::TDevice_Event evt;

	std::set<GUID> signalsBeingReset;

	mInputData.clear();
	size_t i = 0;

	// TODO: get rid of excessive locking (mutexes)

	while (mInput->receive(&evt) == S_OK)
	{
		// incoming level or calibration - store to appropriate vector
		if (evt.event_code == glucose::NDevice_Event_Code::Level || evt.event_code == glucose::NDevice_Event_Code::Calibrated)
		{
			std::unique_lock<std::mutex> lck(mChangedMtx);

			mInputData[evt.signal_id].push_back(Value(evt.level, Rat_Time_To_Unix_Time(evt.device_time), evt.segment_id));
			// signal is not being reset (recalculated and resent) right now - set changed flag
			if (signalsBeingReset.find(evt.signal_id) == signalsBeingReset.end())
				mChanged = true;

			// for now, update maximum value just from these signals
			if (evt.signal_id == glucose::signal_IG || evt.signal_id == glucose::signal_BG)
			{
				if (evt.level > mGraphMaxValue)
					mGraphMaxValue = evt.level;
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
			if (refcnt::WChar_Container_Equals_WString(evt.info, rsParameters_Reset))
			{
				// TODO: verify, if parameter reset came for signal, that is really a calculated signal (not measured)

				// incoming parameters just erases the signals, because a new set of calculated levels comes through the pipe
				std::unique_lock<std::mutex> lck(mChangedMtx);
				// remove just segment, for which the reset was issued
				mInputData[evt.signal_id].erase(
					std::remove_if(mInputData[evt.signal_id].begin(), mInputData[evt.signal_id].end(), [evt](Value const& a) { return a.segment_id == evt.segment_id; }),
					mInputData[evt.signal_id].end()
				);
				// remove also markers
				mParameterChanges[evt.signal_id].erase(
					std::remove_if(mParameterChanges[evt.signal_id].begin(), mParameterChanges[evt.signal_id].end(), [evt](Value const& a) { return a.segment_id == evt.segment_id; }),
					mParameterChanges[evt.signal_id].end()
				);

				signalsBeingReset.insert(evt.signal_id);
			}
			else if (refcnt::WChar_Container_Equals_WString(evt.info, rsSegment_Recalculate_Complete))
			{
				signalsBeingReset.erase(evt.signal_id);
				mChanged = true;
			}
		}
		else if (evt.event_code == glucose::NDevice_Event_Code::Time_Segment_Start || evt.event_code == glucose::NDevice_Event_Code::Time_Segment_Stop)
		{
			mSegmentMarkers.push_back(Value(0, Rat_Time_To_Unix_Time(evt.device_time), evt.segment_id));
		}

		if (mOutput->send(&evt) != S_OK)
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

	// clear instance
	mInstance = nullptr;
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

const std::map<GUID, std::string> Signal_Mapping = {
	{ glucose::signal_IG, "ist" },
	{ glucose::signal_BG, "blood" },
	{ glucose::signal_Calibration, "bloodCalibration" },
	{ glucose::signal_ISIG, "isig" },
	{ glucose::signal_Insulin, "insulin" },
	{ glucose::signal_Carb_Intake, "carbs" },
};

void CDrawing_Filter::Prepare_Drawing_Map()
{
	DataMap vectorsMap;

	for (auto const& mapping : Signal_Mapping)
		vectorsMap[mapping.second] = Data(mInputData[mapping.first], true, mInputData[mapping.first].empty(), false);

	vectorsMap["segment_markers"] = Data(mSegmentMarkers, true, mSegmentMarkers.empty(), false);

	std::string base = "calc";
	size_t curIdx = 0;

	for (auto const& presentData : mInputData)
	{
		if (Signal_Mapping.find(presentData.first) == Signal_Mapping.end())
		{
			std::string key = base + std::to_string(curIdx);

			vectorsMap[key] = Data(mInputData[presentData.first], true, false, true);
			vectorsMap[key].identifier = key;

			if (mParameterChanges.find(presentData.first) != mParameterChanges.end())
			{
				const std::string pkey = "param_" + key;
				vectorsMap[pkey] = Data(mParameterChanges[presentData.first], true, false, false);
				vectorsMap[pkey].identifier = pkey;
			}

			auto itr = mCalcSignalNameMap.find(presentData.first);
			if (itr != mCalcSignalNameMap.end())
				mLocaleMap[key] = itr->second;
			else
				mLocaleMap[key] = "???";

			curIdx++;
		}
	}

	mDataMap = vectorsMap;
}

HRESULT CDrawing_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration)
{
	CDrawing_Filter* trg = nullptr;
	if (!mInstance.compare_exchange_strong(trg, this))
	{
		std::wcerr << L"More than one instance of drawing filter initialized!" << std::endl;
		return E_FAIL;
	}

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
	if (configuration->get(&cbegin, &cend) != S_OK)
		return E_FAIL;

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
	}

	if (mCanvasWidth != 0 && mCanvasHeight != 0)
	{
		CAGP_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CClark_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CDay_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CGraph_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CParkes_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
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

	mRunning = true;

	mSchedulerThread = std::make_unique<std::thread>(&CDrawing_Filter::Run_Scheduler, this);

	Run_Main();

	return S_OK;
}

CDrawing_Filter* CDrawing_Filter::Get_Instance()
{
	return mInstance;
}

inline std::string WToMB(std::wstring in)
{
	return std::string(in.begin(), in.end());
}

void CDrawing_Filter::Store_To_File(std::string& str, std::wstring& filePath)
{
	std::ofstream ofs(filePath);
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

	glucose::TDevice_Event drawEvt;
	drawEvt.event_code = glucose::NDevice_Event_Code::Information;
	drawEvt.device_time = Unix_Time_To_Rat_Time(time(nullptr));
	drawEvt.info = refcnt::WString_To_WChar_Container(rsInfo_Redraw_Complete);
	//drawEvt.logical_time = 0; // asynchronnous event time
	mOutput->send(&drawEvt);
}

const char* CDrawing_Filter::Get_SVG_AGP() const
{
	std::unique_lock<std::mutex> lck(mRetrieveMtx);

	return mAGP_SVG.c_str();
}

const char* CDrawing_Filter::Get_SVG_Clark() const
{
	std::unique_lock<std::mutex> lck(mRetrieveMtx);

	return mClark_SVG.c_str();
}

const char* CDrawing_Filter::Get_SVG_Day() const
{
	std::unique_lock<std::mutex> lck(mRetrieveMtx);

	return mDay_SVG.c_str();
}

const char* CDrawing_Filter::Get_SVG_Graph() const
{
	std::unique_lock<std::mutex> lck(mRetrieveMtx);

	return mGraph_SVG.c_str();
}

const char* CDrawing_Filter::Get_SVG_Parkes(bool type1) const
{
	std::unique_lock<std::mutex> lck(mRetrieveMtx);

	if (type1)
		return mParkes_type1_SVG.c_str();
	else
		return mParkes_type2_SVG.c_str();
}

extern "C" HRESULT IfaceCalling get_svg_agp(refcnt::wstr_container* target)
{
	if (auto filter = CDrawing_Filter::Get_Instance())
	{
		const char* pdata = filter->Get_SVG_AGP();

		std::wstring image{ pdata, pdata + strlen(pdata) }; // this step won't be necessary after drawing module rework to wchar_t
		target->set(image.data(), image.data() + image.size());

		return S_OK;
	}
	return E_FAIL;
}

extern "C" HRESULT IfaceCalling get_svg_clark(refcnt::wstr_container* target)
{
	if (auto filter = CDrawing_Filter::Get_Instance())
	{
		const char* pdata = filter->Get_SVG_Clark();

		std::wstring image{ pdata, pdata + strlen(pdata) }; // this step won't be necessary after drawing module rework to wchar_t
		target->set(image.data(), image.data() + image.size());

		return S_OK;
	}
	return E_FAIL;
}

extern "C" HRESULT IfaceCalling get_svg_day(refcnt::wstr_container* target)
{
	if (auto filter = CDrawing_Filter::Get_Instance())
	{
		const char* pdata = filter->Get_SVG_Day();

		std::wstring image{ pdata, pdata + strlen(pdata) }; // this step won't be necessary after drawing module rework to wchar_t
		target->set(image.data(), image.data() + image.size());

		return S_OK;
	}
	return E_FAIL;
}

extern "C" HRESULT IfaceCalling get_svg_graph(refcnt::wstr_container* target)
{
	if (auto filter = CDrawing_Filter::Get_Instance())
	{
		const char* pdata = filter->Get_SVG_Graph();

		std::wstring image{ pdata, pdata + strlen(pdata) }; // this step won't be necessary after drawing module rework to wchar_t
		target->set(image.data(), image.data() + image.size());

		return S_OK;
	}
	return E_FAIL;
}

extern "C" HRESULT IfaceCalling get_svg_parkes(refcnt::wstr_container* target)
{
	if (auto filter = CDrawing_Filter::Get_Instance())
	{
		const char* pdata = filter->Get_SVG_Parkes(true);

		std::wstring image{ pdata, pdata + strlen(pdata) }; // this step won't be necessary after drawing module rework to wchar_t
		target->set(image.data(), image.data() + image.size());

		return S_OK;
	}
	return E_FAIL;
}

extern "C" HRESULT IfaceCalling get_svg_parkes_type2(refcnt::wstr_container* target)
{
	if (auto filter = CDrawing_Filter::Get_Instance())
	{
		const char* pdata = filter->Get_SVG_Parkes(false);

		std::wstring image{ pdata, pdata + strlen(pdata) }; // this step won't be necessary after drawing module rework to wchar_t
		target->set(image.data(), image.data() + image.size());

		return S_OK;
	}
	return E_FAIL;
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
}