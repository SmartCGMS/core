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

CLog_Filter::CLog_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{inpipe}, mOutput{outpipe} {
	
	mNew_Log_Records = refcnt::Create_Container_shared<refcnt::wstr_container>(nullptr, nullptr);
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
		mLog.open(log_file_name);

		result = mLog.is_open();
		if (result) {
			//set decimal point and write the header
			mLog.imbue(std::locale(std::cout.getloc(), new logger::DecimalSeparator<char>('.')));
			mLog << dsLog_Header << std::endl;
		}
	}

	return result;
}

HRESULT CLog_Filter::Run(glucose::IFilter_Configuration* configuration) {

	// load model descriptors to be able to properly format log outputs of parameters	
	mModelDescriptors = glucose::get_model_descriptors();
	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, glucose::IFilter_Configuration>(configuration, true);
	const bool log_opened = Open_Log(shared_configuration);

	for (; glucose::UDevice_Event evt = mInput.Receive(); evt) {
		if (log_opened)
			Log_Event(evt);

		if (!mOutput.Send(evt)) break;
	}	

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
	if (evt.signal_id != Invalid_GUID) log_line << glucose::Signal_Id_To_WStr(evt.signal_id); log_line << delim;
	if (evt.is_level_event()) log_line << evt.level;
		else if (evt.is_info_event()) log_line << refcnt::WChar_Container_To_WString(evt.info.get());
			else if (evt.is_parameters_event()) log_line << Parameters_To_WStr(evt);
	log_line << delim;
	log_line << evt.segment_id << delim;
	log_line << static_cast<size_t>(evt.event_code) << delim;
	log_line << GUID_To_WString(evt.device_id) << delim;
	log_line << GUID_To_WString(evt.signal_id) << std::endl;

	const std::wstring log_line_str = log_line.str();
	mLog << log_line_str;


	auto container = refcnt::WString_To_WChar_Container(log_line_str.c_str());
	mNew_Log_Records->add(container, container+1);
}

HRESULT IfaceCalling CLog_Filter::Pop(refcnt::wstr_list **str) {

	std::unique_lock<std::mutex> scoped_lock{ mLog_Records_Guard };

	decltype(mNew_Log_Records) empty_records_holder = refcnt::Create_Container_shared<refcnt::wstr_container>(nullptr, nullptr);
	
	*str = mNew_Log_Records.get();
	mNew_Log_Records.reset(empty_records_holder.get(), [](refcnt::wstr_list *obj_to_release) {});	//do not actually delete this to retain the internal reference count!	

	return S_OK;
};