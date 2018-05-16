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

namespace logger
{
	// log precision for device_time output
	constexpr std::streamsize DefaultLogPrecision_DeviceTime = 12;
	// log precise for level output
	constexpr std::streamsize DefaultLogPrecision_Level = 8;
	// log precise for model parameter output
	constexpr std::streamsize DefaultLogPrecision_ModelParam = 8;
}

CLog_Filter::CLog_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe)
	: mInput{inpipe}, mOutput{outpipe}
{
	//
}

CLog_Filter::~CLog_Filter()
{
	//
}

void CLog_Filter::Write_Model_Parameters(std::wostream& stream, const glucose::SDevice_Event& evt)
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

void CLog_Filter::Run_Main()
{
	glucose::SDevice_Event evt;
	std::wstring logMsg;

	while (mInput.Receive(evt))
	{
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

HRESULT CLog_Filter::Run(const refcnt::IVector_Container<glucose::TFilter_Parameter> *configuration) {

	wchar_t *begin, *end;

	glucose::TFilter_Parameter *cbegin, *cend;
	if ((configuration != nullptr) && (configuration->get(&cbegin, &cend) == S_OK)) {


		for (glucose::TFilter_Parameter* cur = cbegin; cur < cend; cur += 1)
		{
			if (cur->config_name->get(&begin, &end) != S_OK)
				continue;

			std::wstring confname{ begin, end };

			if (confname == rsLog_Output_File)
			{
				if (cur->wstr->get(&begin, &end) == S_OK)
					mLogFileName = std::wstring(begin, end);
			}
		}

		// if have output file name
		if (!mLogFileName.empty()) {
			// try to open output file
			mLog.open(mLogFileName);
			if (!mLog.is_open())
				return ENOENT;
		}
	}

	Run_Main();

	// load model descriptors to be able to properly format log outputs of parameters	
	mModelDescriptors = glucose::get_model_descriptors();

	return S_OK;
};
