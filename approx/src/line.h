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

#include "../../../common/iface/ApproxIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DeviceLib.h"


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

/*
 * Filter class for computing model parameters
 */
class CLine_Approximator : public glucose::IApproximator, public virtual refcnt::CReferenced {
	protected:
		glucose::WSignal mSignal;

		std::vector<double> mInputTimes, mInputLevels, mSlopes;

		bool Update();

	public:
		CLine_Approximator(glucose::WSignal signal, glucose::IApprox_Parameters_Vector* configuration);
		virtual ~CLine_Approximator() {};

		virtual HRESULT IfaceCalling GetLevels(const double* times, double* const levels, const size_t count, const size_t derivation_order) override;
};

#pragma warning( pop )
