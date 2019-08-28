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
 * Univerzitni 8
 * 301 00, Pilsen
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
class CLog_Filter : public glucose::ISynchronous_Filter, public glucose::ILog_Filter_Inspection, public virtual refcnt::CReferenced {
	protected:
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
		CLog_Filter();
		virtual ~CLog_Filter() = default;
	
		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;

		virtual HRESULT IfaceCalling Configure(glucose::IFilter_Configuration* configuration) override final;
		virtual HRESULT IfaceCalling Execute(glucose::IDevice_Event_Vector* events) override final;

		virtual HRESULT IfaceCalling Pop(refcnt::wstr_list **str) override final;
};

#pragma warning( pop )
