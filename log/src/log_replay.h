#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include <memory>
#include <thread>
#include <fstream>
#include <vector>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for loading previously stored log file and "replay" it through pipe
 */
class CLog_Replay_Filter : public glucose::IFilter, public virtual refcnt::CReferenced
{
	protected:
		glucose::SFilter_Pipe mInput;
		glucose::SFilter_Pipe mOutput;
		std::wifstream mLog;

		std::unique_ptr<std::thread> mLog_Replay_Thread;

	protected:
		// thread method
		void Run_Main();

		// opens log for reading, returns true if success, false if failed
		bool Open_Log(glucose::SFilter_Parameters configuration);
		// converts string to parameters vector; note that this method have no knowledge of models at all (does not validate parameter count, ..)
		void WStr_To_Parameters(const std::wstring& src, glucose::SModel_Parameter_Vector& target);

	public:
		CLog_Replay_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
		virtual ~CLog_Replay_Filter() {};

		virtual HRESULT Run(refcnt::IVector_Container<glucose::TFilter_Parameter>* const configuration) override final;
};

#pragma warning( pop )
