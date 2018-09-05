/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
 */

#pragma once

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/UILib.h"

#include <memory>
#include <thread>
#include <mutex>
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
	glucose::CSignal_Names mSignal_Names;

	bool mIs_Terminated = false;
	
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
	
	virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;

	virtual HRESULT IfaceCalling Run(glucose::IFilter_Configuration* configuration) override final;
	virtual HRESULT IfaceCalling Pop(refcnt::wstr_list **str) override final;
};

#pragma warning( pop )
