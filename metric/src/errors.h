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
		glucose::SFilter_Pipe mInput;
		// output pipe
		glucose::SFilter_Pipe mOutput;

		// currently used error counter instance; TODO: in future, we need to consider more segments at once
		std::unique_ptr<CError_Marker_Counter> mErrorCounter;
	public:
		CErrors_Filter(glucose::SFilter_Pipe inpipe, glucose::SFilter_Pipe outpipe);
		virtual ~CErrors_Filter() {};
		
		virtual HRESULT IfaceCalling QueryInterface(const GUID*  riid, void ** ppvObj) override final;

		virtual HRESULT IfaceCalling Run(glucose::IFilter_Configuration* configuration) override;

		// retrieves the only instance of errors filter
		virtual HRESULT IfaceCalling Get_Errors(const GUID *signal_id, const glucose::NError_Type type, glucose::TError_Markers *markers) override final;
};


#pragma warning( pop )
