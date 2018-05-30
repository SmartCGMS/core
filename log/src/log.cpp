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

namespace logger {

	template<typename CharT>
	class DecimalSeparator : public std::numpunct<CharT>
	{
	public:
		DecimalSeparator(CharT Separator)
			: m_Separator(Separator)
		{}

	protected:
		CharT do_decimal_point()const
		{
			return m_Separator;
		}

	private:
		CharT m_Separator;
	};


	// log precision for device_time output
	//constexpr std::streamsize DefaultLogPrecision_DeviceTime = 12;
	// log precise for level output
	//constexpr std::streamsize DefaultLogPrecision_Level = 8;
	// log precise for model parameter output
	//constexpr std::streamsize DefaultLogPrecision_ModelParam = 8;
}

CLog_Filter::CLog_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{inpipe}, mOutput{outpipe} {
	
}

std::wstring CLog_Filter::Parameters_To_WStr(const glucose::UDevice_Event& evt) {
	// find appropriate model descriptor by id
	glucose::TModel_Descriptor* modelDesc = nullptr;
	for (auto& desc : mModelDescriptors)
	{
		for (size_t i = 0; i < desc.number_of_calculated_signals; i++)
		{
			if (evt.signal_id == desc.calculated_signal_ids[i])
			{
				modelDesc = &desc;
				break;
			}
		}

		if (modelDesc)
			break;
	}

	if (!modelDesc)
		return rsCannot_Get_Parameters;

	// retrieve params
	double *begin, *end;
	if (evt.parameters->get(&begin, &end) != S_OK)
		return rsCannot_Get_Parameters;

	// format output
	std::wostringstream stream;
	for (size_t i = 0; i < modelDesc->number_of_parameters; i++) {
		if (i > 0) stream << L", ";
		stream << modelDesc->parameter_ui_names[i] << "=" /*<< std::setprecision(logger::DefaultLogPrecision_ModelParam) */<< begin[i];
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

HRESULT CLog_Filter::Run(refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) {

	// load model descriptors to be able to properly format log outputs of parameters	
	mModelDescriptors = glucose::get_model_descriptors();
	glucose::SFilter_Parameters shared_configuration = refcnt::make_shared_reference_ext<glucose::SFilter_Parameters, refcnt::IVector_Container<glucose::TFilter_Parameter>>(configuration, true);
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
		
	mLog << evt.logical_time << delim;
	mLog << Rat_Time_To_Local_Time_WStr(evt.device_time, rsLog_Date_Time_Format) << delim;
	mLog << glucose::event_code_text[static_cast<size_t>(evt.event_code)] << delim;
	mLog << glucose::Signal_Id_To_WStr(evt.signal_id) << delim;
	if (evt.is_level_event()) mLog << evt.level; mLog << delim;
	if (evt.is_info_event()) mLog << refcnt::WChar_Container_To_WString(evt.info.get()); mLog << delim;
	if (evt.is_parameters_event()) mLog << Parameters_To_WStr(evt); mLog << delim;
	mLog << evt.segment_id << delim;
	mLog << static_cast<size_t>(evt.event_code) << delim;
	mLog << GUID_To_WString(evt.device_id) << delim;
	mLog << GUID_To_WString(evt.signal_id) << std::endl;
}