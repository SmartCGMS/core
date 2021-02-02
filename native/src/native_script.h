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
 * Univerzitni 8, 301 00 Pilsen
 * Czech Republic
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

#include "native_segment.h"

#include "../../../common/rtl/Dynamic_Library.h"

#include <map>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CNative_Script : public virtual scgms::CBase_Filter {
protected:
    std::map<uint64_t, CNative_Segment> mSegments;    
    std::array<GUID, native::max_signal_count> mSignal_Ids{0};
    std::array<double, native::max_parameter_count> mParameters{ 0.0 };
    std::map<GUID, size_t> mSignal_To_Ordinal;    
protected:
    CDynamic_Library mDll;
    TNative_Execute_Wrapper mEntry_Point = nullptr;
    size_t mCustom_Data_Size = 0;
protected:
    // scgms::CBase_Filter iface implementation
    virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
    virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;
public:
    CNative_Script(scgms::IFilter* output);
    virtual ~CNative_Script() {}
};

#pragma warning( pop )