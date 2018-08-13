#include "log.h"

#include "../../../common/iface/DeviceIface.h"
#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/UILib.h"
#include "../../../common/rtl/rattime.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <ctime>
#include <vector>
#include <string>
#include <map>
#include <codecvt>

CLog_Filter::CLog_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{inpipe}, mOutput{outpipe} {
	
	mNew_Log_Records = refcnt::Create_Container_shared<refcnt::wstr_container*>(nullptr, nullptr);
}

HRESULT IfaceCalling CLog_Filter::QueryInterface(const GUID*  riid, void ** ppvObj) {
	if (Internal_Query_Interface<glucose::IFilter>(glucose::Log_Filter, *riid, ppvObj)) return S_OK;
	if (Internal_Query_Interface<glucose::ILog_Filter_Inspection>(glucose::Log_Filter_Inspection, *riid, ppvObj)) return S_OK;

	return E_NOINTERFACE;
}

std::wstring CLog_Filter::Signal_Id_To_WStr(const GUID &signal_id) {
	const std::map<GUID, const wchar_t*> signal_names = {	//don't make it static unless with TBB allocator, else the memory won't be freed
		{ glucose::signal_BG, L"BG" },
		{ glucose::signal_IG, L"IG" },
		{ glucose::signal_ISIG, L"ISIG" },
		{ glucose::signal_Calibration, L"Calibration" },
		{ glucose::signal_Insulin, L"Insulin" },
		{ glucose::signal_Carb_Intake, L"Carb" },
		{ glucose::signal_Health_Stress, L"Stress" },
		{ glucose::signal_Diffusion_v2_Blood, L"Diff2 BG" },
		{ glucose::signal_Diffusion_v2_Ist, L"Diff2 IG" },
		{ glucose::signal_Steil_Rebrin_Blood, L"SR BG" }
	};

	const auto resolved_name = signal_names.find(signal_id);
	if (resolved_name != signal_names.end()) return resolved_name->second;
	else return GUID_To_WString(signal_id);
}

std::wstring CLog_Filter::Parameters_To_WStr(const glucose::UDevice_Event& evt) {
	// retrieve params
	double *begin, *end;
	if (evt.parameters->get(&begin, &end) != S_OK)
		return rsCannot_Get_Parameters;


	// find appropriate model descriptor by id
	glucose::TModel_Descriptor* modelDesc = nullptr;
	for (auto& desc : mModelDescriptors)
	{
		for (size_t i = 0; i < desc.number_of_calculated_signals; i++) 	{
			if (evt.signal_id == desc.calculated_signal_ids[i]) {
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
			stream << modelDesc->parameter_ui_names[i] << "=" /*<< std::setprecision(logger::DefaultLogPrecision_ModelParam) */ << begin[i];
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

bool CLog_Filter::Open_Log(glucose::SFilter_Parameters configuration) {

	bool result = false;
	std::wstring log_file_name = configuration.Read_String(rsLog_Output_File);
	// if have output file name
	if (!log_file_name.empty()) {
		// try to open output file
		std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
		mLog.open(converterX.to_bytes(log_file_name));

		result = mLog.is_open();
		if (result) {
			//set decimal point and write the header
			mLog.imbue(std::locale(std::cout.getloc(), new logger::DecimalSeparator<char>('.')));
			mLog << dsLog_Header << std::endl;
		}
	}

	return result;
}

HRESULT IfaceCalling CLog_Filter::Run(glucose::IFilter_Configuration* configuration) {
	mIs_Terminated = false;

	// load model descriptors to be able to properly format log outputs of parameters	
	mModelDescriptors = glucose::get_model_descriptors();
	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);
	const bool log_opened = Open_Log(shared_configuration);

	for (; glucose::UDevice_Event evt = mInput.Receive(); ) {
		if (log_opened)
			Log_Event(evt);

		if (!mOutput.Send(evt)) break;
	}	

	mIs_Terminated = true;

	if (log_opened)
		mLog.close();

	return S_OK;
};


void CLog_Filter::Log_Event(const glucose::UDevice_Event &evt) {
	const wchar_t *delim = L"; ";
		
	std::wostringstream log_line;


	log_line << evt.logical_time << delim;
	log_line << Rat_Time_To_Local_Time_WStr(evt.device_time, rsLog_Date_Time_Format) << delim;
	log_line << glucose::event_code_text[static_cast<size_t>(evt.event_code)] << delim;
	if (evt.signal_id != Invalid_GUID) log_line << Signal_Id_To_WStr(evt.signal_id);
		log_line << delim;
	if (evt.is_level_event()) log_line << evt.level;
		else if (evt.is_info_event()) log_line << refcnt::WChar_Container_To_WString(evt.info.get());
			else if (evt.is_parameters_event()) log_line << Parameters_To_WStr(evt);
	log_line << delim;
	log_line << evt.segment_id << delim;
	log_line << static_cast<size_t>(evt.event_code) << delim;
	log_line << GUID_To_WString(evt.device_id) << delim;
	log_line << GUID_To_WString(evt.signal_id);
	// note the absence of std::endl at the end of line - we include it in file output only (see below)
	// but not in GUI output, since records in list are considered "lines" and external logic (e.g. GUI) maintains line endings by itself

	const std::wstring log_line_str = log_line.str();
	mLog << log_line_str << std::endl;

	refcnt::wstr_container* container = refcnt::WString_To_WChar_Container(log_line_str.c_str());
	std::unique_lock<std::mutex> scoped_lock{ mLog_Records_Guard };
	mNew_Log_Records->add(&container, &container +1);
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
