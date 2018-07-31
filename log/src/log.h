#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"

#include <memory>
#include <thread>
#include <fstream>
#include <vector>
#include <mutex>

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

}

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for logging all incoming events and dropping them (terminating the chain)
 */
class CLog_Filter : public glucose::IFilter, public glucose::ILog_Filter_Inspection, public virtual refcnt::CReferenced {
protected:
	glucose::SFilter_Pipe mInput;
	glucose::SFilter_Pipe mOutput;
	std::wofstream mLog;
	
	std::mutex mLog_Records_Guard;
	std::shared_ptr<refcnt::wstr_list> mNew_Log_Records;
	
	bool Open_Log(glucose::SFilter_Parameters configuration);
	void Log_Event(const glucose::UDevice_Event &evt);
protected:
	// vector of model descriptors; stored for parameter formatting
	std::vector<glucose::TModel_Descriptor> mModelDescriptors;
	std::wstring Parameters_To_WStr(const glucose::UDevice_Event& evt);
public:
	CLog_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
	virtual ~CLog_Filter() {};

	virtual HRESULT Run(glucose::IFilter_Configuration* configuration) override final;
	virtual HRESULT IfaceCalling Pop(refcnt::wstr_list **str) override final;
};

#pragma warning( pop )
