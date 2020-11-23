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

#include "log.h"

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/rattime.h"
#include "../../../common/utils/string_utils.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <ctime>
#include <vector>
#include <string>
#include <map>

CLog_Filter::CLog_Filter(scgms::IFilter *output) : CBase_Filter(output) {
	mNew_Log_Records = refcnt::Create_Container_shared<refcnt::wstr_container*>(nullptr, nullptr);
}

CLog_Filter::~CLog_Filter() {
	mIs_Terminated = true;

	if (mLog.is_open())
		mLog.close();	
}

HRESULT IfaceCalling CLog_Filter::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<scgms::IFilter>(scgms::IID_Log_Filter, *riid, ppvObj)) return S_OK;
	if (Internal_Query_Interface<scgms::ILog_Filter_Inspection>(scgms::IID_Log_Filter_Inspection, *riid, ppvObj)) return S_OK;

	return E_NOINTERFACE;
}

std::wstring CLog_Filter::Parameters_To_WStr(const scgms::UDevice_Event& evt) {
	// retrieve params
	double *begin, *end;
	if (evt.parameters->get(&begin, &end) != S_OK)
		return rsCannot_Get_Parameters;


	// find appropriate model descriptor by id
	scgms::TModel_Descriptor* modelDesc = nullptr;
	for (auto& desc : mModelDescriptors)
	{
		for (size_t i = 0; i < desc.number_of_calculated_signals; i++) 	{
			if (evt.signal_id() == desc.calculated_signal_ids[i]) {
				modelDesc = &desc;
				break;
			}
		}

		if (modelDesc)
			break;
	}

	// format output
	std::wostringstream stream;
	if (modelDesc) {

		for (size_t i = 0; i < modelDesc->number_of_parameters; i++) {
			if (i > 0) stream << L", ";
			stream << modelDesc->parameter_ui_names[i] << "=" << begin[i];
		}
	}
	else {
		for (auto iter = begin; iter != end; iter++) {
			if (iter != begin) stream << L", ";
			stream << *begin;
		}
	}

	return stream.str();
}

bool CLog_Filter::Open_Log(const filesystem::path &log_filename) {
	bool result = false;

	// if have output file name
	if (!log_filename.empty()) {
		// try to open output file
		mLog.open(log_filename);
	
		result = mLog.is_open();
		if (result) {
			//unused keeps static analysis happy about creating an unnamed object
			auto unused = mLog.imbue(std::locale(std::cout.getloc(), new CDecimal_Separator<char>{ '.' })); //locale takes owner ship of dec_sep
			mLog << dsLog_Header << std::endl;
		}
	}

	return result;
}

HRESULT IfaceCalling CLog_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) {
	// load model descriptors to be able to properly format log outputs of parameters
#ifdef ANDROID
	mLog_Filename = configuration.Read_String(rsLog_Output_File, false);
#else
	mModelDescriptors = scgms::get_model_descriptors();
	mLog_Filename = configuration.Read_File_Path(rsLog_Output_File);
#endif

	HRESULT rc = S_OK;
	if (!mLog_Filename.empty()) {			

		if (!Open_Log(mLog_Filename)) {
			error_description.push(dsCannot_Open_File + mLog_Filename.wstring());

			rc = MK_E_CANTOPENFILE;
		}

	} //empty log file path just means that we simply do not write the log to the disk
	  //but collect it in the memory, e.g.; for GUI	

	mIs_Terminated = false;
	return rc;
}

HRESULT IfaceCalling CLog_Filter::Do_Execute(scgms::UDevice_Event event) {
	Log_Event(event);
	return mOutput.Send(event);
};


void CLog_Filter::Log_Event(const scgms::UDevice_Event &evt) {
	const wchar_t *delim = L"; ";

	std::wostringstream log_line;

	log_line << evt.logical_time() << delim;
	log_line << Rat_Time_To_Local_Time_WStr(evt.device_time(), rsLog_Date_Time_Format) << delim;
	log_line << scgms::event_code_text[static_cast<size_t>(evt.event_code())] << delim;
	if (evt.signal_id() != Invalid_GUID) log_line << mSignal_Names.Get_Name(evt.signal_id());
	log_line << delim;
	if (evt.is_level_event()) log_line << evt.level();
	else if (evt.is_info_event()) log_line << refcnt::WChar_Container_To_WString(evt.info.get());
	else if (evt.is_parameters_event()) log_line << Parameters_To_WStr(evt);
	log_line << delim;
	log_line << evt.segment_id() << delim;
	log_line << static_cast<size_t>(evt.event_code()) << delim;
	log_line << GUID_To_WString(evt.device_id()) << delim;
	log_line << GUID_To_WString(evt.signal_id());
	// note the absence of std::endl at the end of line - we include it in file output only (see below)
	// but not in GUI output, since records in list are considered "lines" and external logic (e.g. GUI) maintains line endings by itself

	const std::wstring log_line_str = log_line.str();
	if (mLog.is_open()) mLog << log_line_str << std::endl;

	refcnt::wstr_container* container = refcnt::WString_To_WChar_Container(log_line_str.c_str());
	{
		std::unique_lock<std::mutex> scoped_lock{ mLog_Records_Guard };
		mNew_Log_Records->add(&container, &container + 1);
	}
	container->Release();
}

HRESULT IfaceCalling CLog_Filter::Pop(refcnt::wstr_list **str) {
	if (mIs_Terminated) return E_FAIL;

	std::unique_lock<std::mutex> scoped_lock{ mLog_Records_Guard };

	if (mNew_Log_Records->empty() == S_OK) {
		*str = nullptr;
		return S_FALSE;
	}
	else {
		*str = mNew_Log_Records.get();
		(*str)->AddRef();

		mNew_Log_Records = refcnt::Create_Container_shared<refcnt::wstr_container*>(nullptr, nullptr);

		return S_OK;
	}
};