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
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
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


#include "drawing_filter.h"

#include "../../../common/lang/dstrings.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/utils/string_utils.h"

#include "drawing/Generators/IGenerator.h"
#include "drawing/Generators/GraphGenerator.h"
#include "drawing/Generators/ClarkGenerator.h"
#include "drawing/Generators/ParkesGenerator.h"
#include "drawing/Generators/AGPGenerator.h"
#include "drawing/Generators/DayGenerator.h"
#include "drawing/Generators/ECDFGenerator.h"
#include "drawing/Generators/Mobile_GlucoseGenerator.h"
#include "drawing/Generators/Mobile_CarbsGenerator.h"
#include "drawing/Generators/Mobile_InsulinGenerator.h"

#include <map>

#include <iostream>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <set>

#undef min

const std::map<GUID, const char*, std::less<GUID>> Signal_Mapping = {
	{ scgms::signal_IG, "ist" },
	{ scgms::signal_BG, "blood" },
	{ scgms::signal_Calibration, "bloodCalibration" },
	{ scgms::signal_ISIG, "isig" },
	{ scgms::signal_Delivered_Insulin_Bolus, "insulin" },
	{ scgms::signal_Requested_Insulin_Bolus, "calcd_insulin" },
	{ scgms::signal_Delivered_Insulin_Basal_Rate, "basal_insulin" },
	{ scgms::signal_Requested_Insulin_Basal_Rate, "basal_insulin_rate" },
	{ scgms::signal_Insulin_Activity, "insulin_activity" },
	{ scgms::signal_Carb_Intake, "carbs" },
	{ scgms::signal_IOB, "iob" },
	{ scgms::signal_COB, "cob" },	
};

CDrawing_Filter::CDrawing_Filter(scgms::IFilter *output) : CBase_Filter(output), mGraphMaxValue(-1) {
	//
}

HRESULT IfaceCalling CDrawing_Filter::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<scgms::IFilter>(scgms::IID_Drawing_Filter, *riid, ppvObj)) return S_OK;
	if (Internal_Query_Interface<scgms::IDrawing_Filter_Inspection>(scgms::IID_Drawing_Filter_Inspection, *riid, ppvObj)) return S_OK;

	return E_NOINTERFACE;
}

HRESULT CDrawing_Filter::Do_Execute(scgms::UDevice_Event event) {
	
	std::unique_lock<std::mutex> lck(mChangedMtx);
	mChanged = true;
	mChangedInternal = true;

	// incoming level or calibration - store to appropriate vector
	if (event.event_code() == scgms::NDevice_Event_Code::Level )
	{
		//mInputData[event.signal_id()][event.segment_id()].push_back(Value(event.level(), Rat_Time_To_Unix_Time(event.device_time()), event.segment_id()));
		auto &data = mInputData[event.signal_id()][event.segment_id()];
		double insert_time = event.device_time();
		if (!data.empty()) {			
			const auto last_time = data[data.size() - 1].date;
			if (last_time != Rat_Time_To_Unix_Time(insert_time))
				data.push_back(Value(event.level(), Rat_Time_To_Unix_Time(insert_time), event.segment_id()));
			else {
				data[data.size() - 1].Set_Value(event.level());
			}
		} else
			data.push_back(Value(event.level(), Rat_Time_To_Unix_Time(insert_time), event.segment_id()));
		
			 
	}
	// incoming new parameters
	else if (event.event_code() == scgms::NDevice_Event_Code::Parameters)
	{
		mParameterChanges[event.signal_id()][event.segment_id()].push_back(Value((double)Rat_Time_To_Unix_Time(event.device_time()), Rat_Time_To_Unix_Time(event.device_time()), event.segment_id()));
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Shut_Down)
	{
		// when the filter shuts down, store drawings to files
		Force_Redraw();

		Store_To_File(mGraph_SVG, mGraph_FilePath);
		Store_To_File(mClark_SVG, mClark_FilePath);
		Store_To_File(mDay_SVG, mDay_FilePath);
		Store_To_File(mAGP_SVG, mAGP_FilePath);
		Store_To_File(mParkes_type1_SVG, mParkes_FilePath);
		Store_To_File(mECDF_SVG, mECDF_FilePath);
		// TODO: store profile drawings
	}
	// incoming parameters reset information message
	else if (event.event_code() == scgms::NDevice_Event_Code::Information)
	{
		// we catch parameter reset information message
		if (event.info == rsParameters_Reset)
		{
			// TODO: verify, if parameter reset came for signal, that is really a calculated signal (not measured)

			// reset of specific signal
			if (event.signal_id() != Invalid_GUID)
			{
				Reset_Signal(event.signal_id(), event.segment_id());
				mSignalsBeingReset.insert(event.signal_id());
			}
			else // reset of all calculated signals (signal == Invalid_GUID)
			{
				for (auto& data : mInputData)
				{
					if (Signal_Mapping.find(data.first) == Signal_Mapping.end())
					{
						Reset_Signal(data.first, event.segment_id());
						mSignalsBeingReset.insert(event.signal_id());
					}
				}
			}
		}
		else if (event.info == rsSegment_Recalculate_Complete)
		{
			mSignalsBeingReset.erase(event.signal_id());
//			mChanged = true;
		}
		else if (refcnt::WChar_Container_Equals_WString(event.info.get(), L"DrawingResize", 0, 13))
		{
			std::wstring str = refcnt::WChar_Container_To_WString(event.info.get());
			auto rpos = str.find(L'=');
			auto cpos = str.find(L',');
				
			auto drawingId = static_cast<scgms::TDrawing_Image_Type>(std::stoull(str.substr(rpos + 1, rpos - cpos - 1)));
			str = str.substr(cpos + 1);
			rpos = str.find(L'=');
			cpos = str.find(L',');

			int width = static_cast<int>(std::stoull(str.substr(rpos + 1, rpos - cpos - 1)));
			str = str.substr(cpos + 1);
			rpos = str.find(L'=');

			int height = static_cast<int>(std::stoull(str.substr(rpos + 1)));

			switch (drawingId)
			{
				case scgms::TDrawing_Image_Type::Graph:
					CGraph_Generator::Set_Canvas_Size(width, height);
					break;
				case scgms::TDrawing_Image_Type::Day:
					CDay_Generator::Set_Canvas_Size(width, height);
					break;
				case scgms::TDrawing_Image_Type::Parkes:
					CParkes_Generator::Set_Canvas_Size(width, height);
					break;
				case scgms::TDrawing_Image_Type::Clark:
					CClark_Generator::Set_Canvas_Size(width, height);
					break;
				case scgms::TDrawing_Image_Type::AGP:
					CAGP_Generator::Set_Canvas_Size(width, height);
					break;
				case scgms::TDrawing_Image_Type::ECDF:
					CECDF_Generator::Set_Canvas_Size(width, height);
					break;
				case scgms::TDrawing_Image_Type::Profile_Glucose:
					CMobile_Glucose_Generator::Set_Canvas_Size(width, height);
					break;
				case scgms::TDrawing_Image_Type::Profile_Carbs:
					CMobile_Carbs_Generator::Set_Canvas_Size(width, height);
					break;
				case scgms::TDrawing_Image_Type::Profile_Insulin:
					CMobile_Insulin_Generator::Set_Canvas_Size(width, height);
					break;
				default:
					break;
			}
		}
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Time_Segment_Start || event.event_code() == scgms::NDevice_Event_Code::Time_Segment_Stop)
	{
		mSegmentMarkers.push_back(Value(0, Rat_Time_To_Unix_Time(event.device_time()), event.segment_id()));
	}
	else if (event.event_code() == scgms::NDevice_Event_Code::Warm_Reset)
	{
		mInputData.clear();
		mParameterChanges.clear();
	}

	return Send(event);
}

bool CDrawing_Filter::Force_Redraw(const std::unordered_set<uint64_t> &segmentIds, const std::set<GUID> &signalIds)
{
	mChangedInternal = false;

	mDataMap.clear();
	Prepare_Drawing_Map(segmentIds, signalIds);

	Generate_Graphs(mDataMap, mGraphMaxValue, mLocaleMap);

	return true;
}

bool CDrawing_Filter::Redraw_If_Changed(const std::unordered_set<uint64_t> &segmentIds, const std::set<GUID> &signalIds)
{
	std::unique_lock<std::mutex> lck(mChangedMtx);

	if (mChangedInternal || !segmentIds.empty() || !signalIds.empty())
		return Force_Redraw(segmentIds, signalIds);

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

	vectorsMap["ist"].identifier = "ist";
	vectorsMap["ist"].refSignalIdentifier = "blood";

	vectorsMap["segment_markers"] = Data(mSegmentMarkers, true, mSegmentMarkers.empty(), false);

	std::string base = "calc";
	size_t curIdx = 0;

	std::map<GUID, std::string> calcSignalMap;

	for (auto const& presentData : mInputData)
	{
		// do not use signals we don't want to draw
		if (!allSignals && signalIds.find(presentData.first) == signalIds.end())
			continue;

		if (Signal_Mapping.find(presentData.first) == Signal_Mapping.end() && calcSignalMap.find(presentData.first) == calcSignalMap.end())
			calcSignalMap[presentData.first] = base + std::to_string(curIdx++);

		const GUID& signal_id = presentData.first;

		if (Signal_Mapping.find(presentData.first) == Signal_Mapping.end())
		{
			//const std::string& key = calcSignalMap[presentData.first];
			const std::string& key = Narrow_WString(GUID_To_WString(presentData.first));
								

			mGraphMaxValue = 0.0;

			vectorsMap[key] = Data({}, true, false, true);
			for (auto& dataPair : mInputData[signal_id])
			{
				if (allSegments || segmentIds.find(dataPair.first) != segmentIds.end())
				{
					const auto& data = dataPair.second;
					vectorsMap[key].values.reserve(vectorsMap[key].values.size() + data.size());
					for (auto& val : data) {
						vectorsMap[key].values.push_back(val);

						// several signals are excluded from maximum value determining, since their values are not in mmol/l, and are drawn in a different way
						// TODO: more generic way to determine value units
						if (presentData.first != scgms::signal_Carb_Intake && presentData.first != scgms::signal_Delivered_Insulin_Bolus && presentData.first != scgms::signal_Delivered_Insulin_Basal_Rate && presentData.first != scgms::signal_Physical_Activity
							&& presentData.first != scgms::signal_ISIG && presentData.first != scgms::signal_COB && presentData.first != scgms::signal_IOB)
						{
							if (val.value > mGraphMaxValue)
								mGraphMaxValue = std::min(val.value, 150.0); //http://www.guinnessworldrecords.com/world-records/highest-blood-sugar-level/
						} 
					}
				}
			}

			auto refItr = mReferenceForCalcMap.find(presentData.first);
			if (refItr != mReferenceForCalcMap.end())
			{
				auto nameMeasuredItr = Signal_Mapping.find(refItr->second);
				if (nameMeasuredItr != Signal_Mapping.end())
					vectorsMap[key].refSignalIdentifier = nameMeasuredItr->second;
				else
				{
					auto nameCalcItr = calcSignalMap.find(refItr->second);
					if (nameCalcItr != calcSignalMap.end())
						vectorsMap[key].refSignalIdentifier = nameCalcItr->second;
				}
			}

			vectorsMap[key].identifier = key;
			vectorsMap[key].signal_id = presentData.first;

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
				vectorsMap[pkey].signal_id = presentData.first;
			}

			auto itr = mCalcSignalNameMap.find(signal_id);
			if (itr != mCalcSignalNameMap.end())
				mLocaleMap[key] = itr->second;
			else
				mLocaleMap[key] = Narrow_WString(GUID_To_WString(signal_id));
		}
	}

	for (auto& mapping : vectorsMap)
	{
		std::sort(mapping.second.values.begin(), mapping.second.values.end(), [](Value& a, Value& b) {
			return a.date < b.date;
		});
	}

	mDataMap = vectorsMap;
}

HRESULT CDrawing_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	mCanvasWidth = static_cast<int>(configuration.Read_Int(rsDrawing_Filter_Canvas_Width, mCanvasWidth));
	mCanvasHeight = static_cast<int>(configuration.Read_Int(rsDrawing_Filter_Canvas_Height, mCanvasHeight));
	mAGP_FilePath = configuration.Read_File_Path(rsDrawing_Filter_Filename_AGP);
	mClark_FilePath = configuration.Read_File_Path(rsDrawing_Filter_Filename_Clark);
	mDay_FilePath = configuration.Read_File_Path(rsDrawing_Filter_Filename_Day);
	mGraph_FilePath = configuration.Read_File_Path(rsDrawing_Filter_Filename_Graph);
	mParkes_FilePath = configuration.Read_File_Path(rsDrawing_Filter_Filename_Parkes);
	mECDF_FilePath = configuration.Read_File_Path(rsDrawing_Filter_Filename_ECDF);

	if (mCanvasWidth != 0 && mCanvasHeight != 0)
	{
		CAGP_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CClark_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CDay_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CGraph_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CParkes_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CECDF_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CMobile_Glucose_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CMobile_Carbs_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
		CMobile_Insulin_Generator::Set_Canvas_Size(mCanvasWidth, mCanvasHeight);
	}

	// cache model signal names
	auto models = scgms::get_model_descriptors();
	scgms::CSignal_Description signal_descriptions;
	signal_descriptions.for_each([this](scgms::TSignal_Descriptor desc) {
		mCalcSignalNameMap[desc.id] = Narrow_WChar(desc.signal_description);
	});

	for (auto const& model : models)
	{
		for (size_t i = 0; i < model.number_of_calculated_signals; i++)
		{
			std::wstring wname = /*std::wstring(model.description) + L" - " + */signal_descriptions.Get_Name(model.calculated_signal_ids[i]);
			mCalcSignalNameMap[model.calculated_signal_ids[i]] = Narrow_WString(wname);
			mReferenceForCalcMap[model.calculated_signal_ids[i]] = model.reference_signal_ids[i];
		}
	}

	//for (size_t i = 0; i < scgms::signal_Virtual.size(); i++)
//		mCalcSignalNameMap[scgms::signal_Virtual[i]] = std::string("virtual ") + std::to_string(i);

	

	Fill_Localization_Map(mLocaleMap);

	mSignalsBeingReset.clear();

	return S_OK;
}

void CDrawing_Filter::Store_To_File(std::string& str, std::wstring& filePath)
{
	if (filePath.empty())
		return;

	std::ofstream ofs(Narrow_WString(filePath));
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
			Set_Locale_Title(locales, dsDrawingLocaleTitleECDF);
			CECDF_Generator graph(valueMap, maxValue, locales, 1);
			mECDF_SVG = graph.Build_SVG();
		}

		// Mobile - glucose generator scope
		{
			Set_Locale_Title(locales, dsDrawingLocaleTitleProfileGlucose);
			CMobile_Glucose_Generator graph(valueMap, maxValue, locales, 1);
			mProfile_Glucose_SVG = graph.Build_SVG();
		}

		// Mobile - bolus carbs generator scope
		{
			Set_Locale_Title(locales, dsDrawingLocaleTitleProfileGlucose);
			CMobile_Carbs_Generator graph(valueMap, maxValue, locales, 1);
			mProfile_Carbs_SVG = graph.Build_SVG();
		}

		// Mobile - glucose generator scope
		{
			Set_Locale_Title(locales, dsDrawingLocaleTitleProfileGlucose);
			CMobile_Insulin_Generator graph(valueMap, maxValue, locales, 1);
			mProfile_Insulin_SVG = graph.Build_SVG();
		}
	}
}

HRESULT CDrawing_Filter::Get_Plot(const std::string &plot, refcnt::IVector_Container<char> *svg) const {
	char* plot_ptr = const_cast<char*>(plot.data());
	return svg->set(plot_ptr, plot_ptr + plot.size());
}


HRESULT IfaceCalling CDrawing_Filter::New_Data_Available() {
	return mChanged.exchange(false) ? S_OK : S_FALSE;
}

HRESULT IfaceCalling CDrawing_Filter::Draw(scgms::TDrawing_Image_Type type, scgms::TDiagnosis diagnosis, refcnt::str_container *svg, refcnt::IVector_Container<uint64_t> *segmentIds, refcnt::IVector_Container<GUID> *signalIds) {	
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
		case scgms::TDrawing_Image_Type::AGP:
			return Get_Plot(mAGP_SVG, svg);
		case scgms::TDrawing_Image_Type::Day:
			return Get_Plot(mDay_SVG, svg);
		case scgms::TDrawing_Image_Type::Graph:
			return Get_Plot(mGraph_SVG, svg);
		case scgms::TDrawing_Image_Type::Clark:
			return Get_Plot(mClark_SVG, svg);
		case scgms::TDrawing_Image_Type::Parkes:
		{
			switch (diagnosis)
			{
				case scgms::TDiagnosis::Type1:
					return Get_Plot(mParkes_type1_SVG, svg);
				case scgms::TDiagnosis::Type2:
				case scgms::TDiagnosis::Gestational:
					return Get_Plot(mParkes_type2_SVG, svg);
					// TODO: Parkes error grid for gestational diabetes
				default:
					break;
			}
		}
		case scgms::TDrawing_Image_Type::ECDF:
			return Get_Plot(mECDF_SVG, svg);
		case scgms::TDrawing_Image_Type::Profile_Glucose:
			return Get_Plot(mProfile_Glucose_SVG, svg);
		case scgms::TDrawing_Image_Type::Profile_Carbs:
			return Get_Plot(mProfile_Carbs_SVG, svg);
		case scgms::TDrawing_Image_Type::Profile_Insulin:
			return Get_Plot(mProfile_Insulin_SVG, svg);
		default:
			break;
	}

	return E_INVALIDARG;
}

void CDrawing_Filter::Set_Locale_Title(LocalizationMap& locales, std::wstring title) const
{
	locales[Narrow_WChar(rsDrawingLocaleTitle)] = Narrow_WString(title);
}

void CDrawing_Filter::Fill_Localization_Map(LocalizationMap& locales)
{


	locales[Narrow_WChar(rsDrawingLocaleTitle)] = Narrow_WChar(dsDrawingLocaleTitle);
	locales[Narrow_WChar(rsDrawingLocaleTitleDay)] = Narrow_WChar(dsDrawingLocaleTitleDay);
	locales[Narrow_WChar(rsDrawingLocaleSubtitle)] = Narrow_WChar(dsDrawingLocaleSubtitle);
	locales[Narrow_WChar(rsDrawingLocaleDiabetes1)] = Narrow_WChar(dsDrawingLocaleDiabetes1);
	locales[Narrow_WChar(rsDrawingLocaleDiabetes2)] = Narrow_WChar(dsDrawingLocaleDiabetes2);
	locales[Narrow_WChar(rsDrawingLocaleTime)] = Narrow_WChar(dsDrawingLocaleTime);
	locales[Narrow_WChar(rsDrawingLocaleTimeDay)] = Narrow_WChar(dsDrawingLocaleTimeDay);
	locales[Narrow_WChar(rsDrawingLocaleConcentration)] = Narrow_WChar(dsDrawingLocaleConcentration);
	locales[Narrow_WChar(rsDrawingLocaleHypoglycemy)] = Narrow_WChar(dsDrawingLocaleHypoglycemy);
	locales[Narrow_WChar(rsDrawingLocaleHyperglycemy)] = Narrow_WChar(dsDrawingLocaleHyperglycemy);
	locales[Narrow_WChar(rsDrawingLocaleBlood)] = Narrow_WChar(dsDrawingLocaleBlood);
	locales[Narrow_WChar(rsDrawingLocaleBloodCalibration)] = Narrow_WChar(dsDrawingLocaleBloodCalibration);
	locales[Narrow_WChar(rsDrawingLocaleIst)] = Narrow_WChar(dsDrawingLocaleIst);
	locales[Narrow_WChar(rsDrawingLocaleResults)] = Narrow_WChar(dsDrawingLocaleResults);
	locales[Narrow_WChar(rsDrawingLocaleDiff2)] = Narrow_WChar(dsDrawingLocaleDiff2);
	locales[Narrow_WChar(rsDrawingLocaleDiff3)] = Narrow_WChar(dsDrawingLocaleDiff3);
	locales[Narrow_WChar(rsDrawingLocaleQuantile)] = Narrow_WChar(dsDrawingLocaleQuantile);
	locales[Narrow_WChar(rsDrawingLocaleRelative)] = Narrow_WChar(dsDrawingLocaleRelative);
	locales[Narrow_WChar(rsDrawingLocaleAbsolute)] = Narrow_WChar(dsDrawingLocaleAbsolute);
	locales[Narrow_WChar(rsDrawingLocaleAverage)] = Narrow_WChar(dsDrawingLocaleAverage);
	locales[Narrow_WChar(rsDrawingLocaleType)] = Narrow_WChar(dsDrawingLocaleType);
	locales[Narrow_WChar(rsDrawingLocaleError)] = Narrow_WChar(dsDrawingLocaleError);
	locales[Narrow_WChar(rsDrawingLocaleDescription)] = Narrow_WChar(dsDrawingLocaleDescription);
	locales[Narrow_WChar(rsDrawingLocaleColor)] = Narrow_WChar(dsDrawingLocaleColor);
	locales[Narrow_WChar(rsDrawingLocaleCounted)] = Narrow_WChar(dsDrawingLocaleCounted);
	locales[Narrow_WChar(rsDrawingLocaleMeasured)] = Narrow_WChar(dsDrawingLocaleMeasured);
	locales[Narrow_WChar(rsDrawingLocaleAxisX)] = Narrow_WChar(dsDrawingLocaleAxisX);
	locales[Narrow_WChar(rsDrawingLocaleAxisY)] = Narrow_WChar(dsDrawingLocaleAxisY);
	locales[Narrow_WChar(rsDrawingLocaleSvgDatetimeTitle)] = Narrow_WChar(dsDrawingLocaleSvgDatetimeTitle);
	locales[Narrow_WChar(rsDrawingLocaleSvgIstTitle)] = Narrow_WChar(dsDrawingLocaleSvgIstTitle);
	locales[Narrow_WChar(rsDrawingLocaleSvgBloodTitle)] = Narrow_WChar(dsDrawingLocaleSvgBloodTitle);
	locales[Narrow_WChar(rsDrawingLocaleSvgBloodCalibrationTitle)] = Narrow_WChar(dsDrawingLocaleSvgBloodCalibrationTitle);
	locales[Narrow_WChar(rsDrawingLocaleSvgInsulinTitle)] = Narrow_WChar(dsDrawingLocaleSvgInsulinTitle);
	locales[Narrow_WChar(rsDrawingLocaleSvgCarbohydratesTitle)] = Narrow_WChar(dsDrawingLocaleSvgCarbohydratesTitle);
	locales[Narrow_WChar(rsDrawingLocaleSvgISIGTitle)] = Narrow_WChar(dsDrawingLocaleSvgISIGTitle);
	locales[Narrow_WChar(rsDrawingLocaleSvgDiff2Title)] = Narrow_WChar(dsDrawingLocaleSvgDiff2Title);
	locales[Narrow_WChar(rsDrawingLocaleSvgDiff3Title)] = Narrow_WChar(dsDrawingLocaleSvgDiff3Title);
	locales[Narrow_WChar(rsDrawingLocaleRelativeError)] = Narrow_WChar(dsDrawingLocaleRelativeError);
	locales[Narrow_WChar(rsDrawingLocaleCummulativeProbability)] = Narrow_WChar(dsDrawingLocaleCummulativeProbability);
	locales[Narrow_WChar(rsDrawingLocaleElevatedGlucose)] = Narrow_WChar(dsDrawingLocaleElevatedGlucose);


	scgms::CSignal_Description signal_descriptions;
	for (auto &sig : Signal_Mapping) {
		locales[sig.second] = Narrow_WString(signal_descriptions.Get_Name(sig.first));
	}
	locales["carbIntake"] = locales["carbs"];
	   
	/*
	locales["basal_insulin_rate"] = "Suggested basal rate";
	locales["cob"] = "Carbohydrates on board";
	locales["iob"] = "Insulin on board";
	locales["insactivity"] = "Insulin activity";
	locales["calcd_insulin"] = "Calculated pre-meal bolus";
	locales["carbIntake"] = "Carbohydrates intake";
	locales["insulin_activity"] = "Insulin activity";
*/	
}
