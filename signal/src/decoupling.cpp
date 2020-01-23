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

#include "decoupling.h"

#include  "expression/expression.h"

#include "../../../common/rtl/FilterLib.h"
#include "../../../common/lang/dstrings.h"

#include <cmath>


CDecoupling_Filter::CDecoupling_Filter(scgms::IFilter *output) : CBase_Filter(output) {
	//
}



HRESULT IfaceCalling CDecoupling_Filter::Do_Configure(scgms::SFilter_Configuration configuration) {
	mSource_Id = configuration.Read_GUID(rsSignal_Source_Id);
	mDestination_Id = configuration.Read_GUID(rsSignal_Destination_Id);
    mRemove_From_Source = configuration.Read_Bool(rsRemove_From_Source, mRemove_From_Source);
       
    mCondition = Parse_AST_Tree(configuration.Read_String(rsCondition));
     
	return mCondition ? S_OK : E_FAIL;
}

HRESULT IfaceCalling CDecoupling_Filter::Do_Execute(scgms::UDevice_Event event) {
    if (event.signal_id() == mSource_Id) {
        const bool decouple = mCondition->evaluate(event).bval;            

        if (decouple) {
            if (mRemove_From_Source) {
                //just change the signal id
                event.signal_id() = mDestination_Id;
                //and send it
            }
            else {
                //we have to clone it
                auto clone = event.Clone();
                clone.signal_id() = mDestination_Id;
                HRESULT rc = Send(clone);
                if (!SUCCEEDED(rc)) return rc;
            }
        }          
    } 
	  
    return Send(event);		
}