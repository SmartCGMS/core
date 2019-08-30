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
#include "../../../common/rtl/SolverLib.h"

#include "error_metric_counter.h"

#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for calculating error metrics
 */
class CErrors_Filter : public glucose::IFilter, public glucose::IError_Filter_Inspection, public virtual refcnt::CReferenced
{
	protected:
		// input pipe
		glucose::SFilter_Pipe_Reader mInput;
		// output pipe
		glucose::SFilter_Pipe_Writer mOutput;

		// currently used error counter instance; TODO: in future, we need to consider more segments at once
		std::unique_ptr<CError_Marker_Counter> mErrorCounter;
	public:
		CErrors_Filter(glucose::SFilter_Pipe_Reader inpipe, glucose::SFilter_Pipe_Writer outpipe);
		virtual ~CErrors_Filter() {};
		
		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;

		virtual HRESULT IfaceCalling Configure(glucose::IFilter_Configuration* configuration) override final;
		virtual HRESULT IfaceCalling Execute() override final;

		// retrieves the only instance of errors filter
		virtual HRESULT IfaceCalling Get_Errors(const GUID *signal_id, const glucose::NError_Type type, glucose::TError_Markers *markers) override final;
};


#pragma warning( pop )
