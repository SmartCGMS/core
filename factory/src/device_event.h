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

#include "../../../common/iface/DeviceIface.h"


class CDevice_Event : public virtual glucose::IDevice_Event {
protected:
	glucose::TDevice_Event mRaw;
public:
	CDevice_Event(glucose::NDevice_Event_Code code);
	virtual ~CDevice_Event();
	virtual ULONG IfaceCalling Release() override;					
	virtual HRESULT IfaceCalling Raw(glucose::TDevice_Event **dst) override;
};

#ifdef _WIN32
	extern "C" __declspec(dllexport) HRESULT IfaceCalling create_device_event(glucose::NDevice_Event_Code code, glucose::IDevice_Event **event);
#else
	extern "C" HRESULT IfaceCalling create_device_event(glucose::NDevice_Event_Code code, glucose::IDevice_Event **event);
#endif
