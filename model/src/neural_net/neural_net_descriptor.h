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

#include "../../../../common/iface/UIIface.h"
#include "../../../../common/rtl/hresult.h"
#include "../../../../common/rtl/ModelsLib.h"
#include "../../../../common/rtl/guid.h"

#include "neural_net.h"

#include <array>

namespace neural_net {
    constexpr double Low_Threshold = 4.0;			//mmol/L below which a medical attention is needed
    constexpr double High_Threshold = 10.0;			//dtto above
    constexpr size_t Internal_Bound_Count = 3;      //number of bounds inside the thresholds

    constexpr double Band_Size = (High_Threshold - Low_Threshold) / static_cast<double>(Internal_Bound_Count);						//must imply relative error <= 10% 
    constexpr double Inv_Band_Size = 1.0 / Band_Size;		//abs(Low_Threshold-Band_Size)/Low_Threshold 
    constexpr double Half_Band_Size = 0.5 / Inv_Band_Size;
    const size_t Band_Count = Internal_Bound_Count + 2;    

    size_t Level_To_Histogram_Index(const double level);
    double Histogram_Index_To_Level(const size_t index);

    constexpr GUID signal_Neural_Net_Prediction = { 0x43607228, 0x6d87, 0x44a4, { 0x84, 0x35, 0xe4, 0xf3, 0xc5, 0xda, 0x62, 0xdd } };	// {43607228-6D87-44A4-8435-E4F3C5DA62DD}	
    extern const scgms::TSignal_Descriptor sig_desc;
}

namespace const_neural_net {      
    //using TOutput = CMulti_Class_Ouput<neural_net::Band_Count>;
    //using TOutput_Layer = CNeural_Layer<5, CSoft_Max, TOutput>;
    using TOutput = CMulti_Label_Ouput<neural_net::Band_Count>;
    using TOutput_Layer = CNeural_Layer<5, CSigmoid, TOutput>;

    using T2nd_Hidden_Layer = CNeural_Layer<5, CTanH, TOutput_Layer>;
    using T1st_Hidden_Layer = CNeural_Layer<5, CTanH, T2nd_Hidden_Layer>;
    using CNeural_Network = T1st_Hidden_Layer;



    constexpr size_t param_count = CNeural_Network::Parameter_Count() + 1;	//+ dt
	struct TParameters {
		union {
			struct {				
				double dt;
                std::array<double, CNeural_Network::Parameter_Count()> neural_parameters;
			};
			std::array<double, param_count> vector;
		};
	};	

    
	extern const TParameters default_parameters;
    scgms::TModel_Descriptor get_calc_desc();
}

namespace reference_neural_net {
    constexpr GUID filter_id = { 0xa2cecd41, 0xd73b, 0x497d, { 0x8a, 0x4c, 0x40, 0x77, 0x8, 0x2b, 0x76, 0x3b } };  // {A2CECD41-D73B-497D-8A4C-4077082B763B}

    extern const scgms::TFilter_Descriptor filter_desc;
}