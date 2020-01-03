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

#include "insulin_bolus.h"
#include "..\..\..\..\common\rtl\SolverLib.h"

CDiscrete_Insulin_Bolus_Calculator::CDiscrete_Insulin_Bolus_Calculator(glucose::IModel_Parameter_Vector* parameters, glucose::IFilter* output) : 
       CBase_Filter(output), mParameters(glucose::Convert_Parameters<insulin_bolus::TParameters>(parameters, insulin_bolus::default_parameters)) {

}


HRESULT CDiscrete_Insulin_Bolus_Calculator::Do_Execute(glucose::UDevice_Event event) {
    if (event.event_code() == glucose::NDevice_Event_Code::Level) {
        if (event.signal_id() == glucose::signal_Carb_Intake) {
            const double bolus_level = event.level() * mParameters.csr; //compute while we still own the event
            HRESULT rc = Send(event);
            if (SUCCEEDED(rc)) {
                glucose::UDevice_Event bolus{ glucose::NDevice_Event_Code::Level };
                bolus.level() = bolus_level;
                bolus.signal_id() = glucose::signal_Requested_Insulin_Bolus;
                rc = Send(bolus);
            }

            return rc;
        }
    }

    return Send(event);
}

HRESULT CDiscrete_Insulin_Bolus_Calculator::Do_Configure(glucose::SFilter_Configuration configuration) {
    // configured in the constructor
    return E_NOTIMPL;
}


HRESULT IfaceCalling CDiscrete_Insulin_Bolus_Calculator::Set_Current_Time(const double new_current_time) {
    return S_OK;    //time independet for now
}

HRESULT IfaceCalling CDiscrete_Insulin_Bolus_Calculator::Step(const double time_advance_delta) {
    return S_OK;    //time independet for now
}