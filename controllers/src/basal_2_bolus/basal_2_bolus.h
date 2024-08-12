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
 * a) This file is available under the Apache License, Version 2.0.
 * b) When publishing any derivative work or results obtained using this software, you agree to cite the following paper:
 *    Tomas Koutny and Martin Ubl, "SmartCGMS as a Testbed for a Blood-Glucose Level Prediction and/or 
 *    Control Challenge with (an FDA-Accepted) Diabetic Patient Simulation", Procedia Computer Science,  
 *    Volume 177, pp. 354-362, 2020
 */

#pragma once

#include <scgms/iface/DeviceIface.h>
#include <scgms/rtl/FilterLib.h>

#include "../descriptor.h"

#include <map>
#include <array>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CBasal_2_Bolus : public virtual scgms::CBase_Filter {
    protected:
        basal_2_bolus::TParameters mParameters{ {{0.0, 0.0} } };

        bool mValid_Settings = false;
        double mInsulin_To_Deliver_Per_Period = 0.0;
        double mEffective_Period = 0.0;
        double mNext_Delivery_Time = 0.0;

        bool Schedule_Delivery(const double current_device_time, const double target_rate, const uint64_t segment_id);
        HRESULT Deliver_Bolus(const double current_device_time, const uint64_t segment_id);

    protected:
        // scgms::CBase_Filter iface implementation
        virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
        virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

    public:

        CBasal_2_Bolus(scgms::IFilter* output);
        virtual ~CBasal_2_Bolus();
};

#pragma warning( pop )