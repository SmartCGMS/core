#include "log.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"
#include "../../../common/rtl/UILib.h"

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
	constexpr std::streamsize DefaultLogPrecision_DeviceTime = 12;
	// log precise for level output
	constexpr std::streamsize DefaultLogPrecision_Level = 8;
	// log precise for model parameter output
	constexpr std::streamsize DefaultLogPrecision_ModelParam = 8;
}

CLog_Filter::CLog_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{inpipe}, mOutput{outpipe} {
	
}

void CLog_Filter::Write_Model_Parameters(std::wostream& stream, const glucose::UDevice_Event& evt)
{
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
		return;

	// retrieve params
	double *begin, *end;
	if (evt.parameters->get(&begin, &end) != S_OK)
		return;

	// format output
	for (size_t i = 0; i < modelDesc->number_of_parameters; i++)
		stream << "," << modelDesc->parameter_ui_names[i] << "=" << std::setprecision(logger::DefaultLogPrecision_ModelParam) << begin[i];
}

void CLog_Filter::Run_Main() {
	std::wstring logMsg;

	for (;  glucose::UDevice_Event evt = mInput.Receive(); evt) {
	
		std::wostringstream logLine;

		// put time in sortable format
		std::time_t t = std::time(nullptr);
		std::tm tm;
		localtime_s(&tm, &t);
		logLine << std::put_time(&tm, L"%Y-%m-%d %H:%M:%S ");

		// always put logical and device time
        //logLine << "logicalTime=" << evt.logical_time << ",";
        logLine << "deviceTime=" << std::setprecision(logger::DefaultLogPrecision_DeviceTime) << evt.device_time << ",";

		// if it is the diagnostic event, prepare log message
		if (evt.event_code == glucose::NDevice_Event_Code::Information || evt.event_code == glucose::NDevice_Event_Code::Warning || evt.event_code == glucose::NDevice_Event_Code::Error)
			logMsg = refcnt::WChar_Container_To_WString(evt.info.get());

		// output may differ with each event type
		switch (evt.event_code)
		{
			case glucose::NDevice_Event_Code::Nothing:
				logLine << "code=Nothing";
				break;
			case glucose::NDevice_Event_Code::Level:
				logLine << "code=Level,deviceGuid=" << GUID_To_WString(evt.device_id) << ",signalGuid=" << GUID_To_WString(evt.signal_id) << ",level=" << std::setprecision(logger::DefaultLogPrecision_Level) << evt.level;
				break;
			case glucose::NDevice_Event_Code::Masked_Level:
				logLine << "code=Masked_Level,deviceGuid=" << GUID_To_WString(evt.device_id) << ",signalGuid=" << GUID_To_WString(evt.signal_id) << ",level=" << std::setprecision(logger::DefaultLogPrecision_Level) << evt.level;
				break;
			case glucose::NDevice_Event_Code::Calibrated:
				logLine << "code=Calibrated,deviceGuid=" << GUID_To_WString(evt.device_id) << ",signalGuid=" << GUID_To_WString(evt.signal_id) << ",level=" << std::setprecision(logger::DefaultLogPrecision_Level) << evt.level;
				break;
			case glucose::NDevice_Event_Code::Parameters:
				logLine << "code=Parameters,signalGuid=" << GUID_To_WString(evt.signal_id);
				Write_Model_Parameters(logLine, evt);
				break;
			case glucose::NDevice_Event_Code::Parameters_Hint:
				logLine << "code=Parameters_Hint,signalGuid=" << GUID_To_WString(evt.signal_id);
				Write_Model_Parameters(logLine, evt);
				break;

			case glucose::NDevice_Event_Code::Suspend_Parameter_Solving:
				logLine << "code=Suspend_Parameter_Solving";
				break;
			case glucose::NDevice_Event_Code::Resume_Parameter_Solving:
				logLine << "code=Resume_Parameter_Solving";
				break;
			case glucose::NDevice_Event_Code::Solve_Parameters:
				logLine << "code=Solve_Parameters";
				break;
			case glucose::NDevice_Event_Code::Time_Segment_Start:
				logLine << "code=Time_Segment_Start";
				break;
			case glucose::NDevice_Event_Code::Time_Segment_Stop:
				logLine << "code=Time_Segment_Stop";
				break;

			case glucose::NDevice_Event_Code::Information:
				logLine << "code=Information,message=" << logMsg;
				break;
			case glucose::NDevice_Event_Code::Warning:
				logLine << "code=Warning,message=" << logMsg;
				break;
			case glucose::NDevice_Event_Code::Error:
				logLine << "code=Error,message=" << logMsg;
				break;
		}

		if (mLog.is_open())
			mLog << logLine.str().c_str() << std::endl; // this also performs flush

		if (!mOutput.Send(evt))
			break;
	}
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

	return S_OK;
};


void CLog_Filter::Log_Event(const glucose::UDevice_Event &evt) {

}