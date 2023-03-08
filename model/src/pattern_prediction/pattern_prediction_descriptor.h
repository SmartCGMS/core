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



namespace pattern_prediction {        
    static constexpr double Relative_Error_Correct_Prediction_Threshold = 0.15;
    static constexpr size_t Relative_Error_Guess_Candidate_Count = 16;   //16 to support vectorization, while having relatively rich number of offsets

    static constexpr double Low_Threshold = 3.0;			//mmol/L below which a medical attention is needed
    static constexpr double High_Threshold = 13.0;			//dtto above
    static constexpr size_t Internal_Bound_Count = 30;      //number of bounds inside the thresholds

    static constexpr double Band_Size = (High_Threshold - Low_Threshold) / static_cast<double>(Internal_Bound_Count);						//must imply relative error <= 10% 
    static constexpr double Inv_Band_Size = 1.0 / Band_Size;		//abs(Low_Threshold-Band_Size)/Low_Threshold 
    static constexpr double Half_Band_Size = 0.5 / Inv_Band_Size;

    static constexpr size_t Band_Count = Internal_Bound_Count + 2;
    static constexpr size_t Subclasses_Count = 3;
    

    static constexpr double Steady_Epsilon = 0.1; //epsilon for considering two levels as equal

    enum class NPattern : size_t {
        accel = 0,                  //a<b<c & acc       ;acc = |c-b|>|b-a|
        up,                         //a<b<c & !acc
        concave,                    //a<b &b>=c

        steady_up,                  //a=b & b<c
        steady,                     //a=b & b=c
        steady_down,                //a=b & b>c

        convex,                     //a>b & b<=c
        down,                       //a>b>c & !acc
        deccel,                     //a>b>c & acc

        count

    };

      constexpr size_t model_param_count = Band_Count * static_cast<size_t>(NPattern::count);

	const GUID filter_id = { 0xa730a576, 0xe84d, 0x4834, { 0x82, 0x6f, 0xfa, 0xee, 0x56, 0x4e, 0x6a, 0xbd } };  // {A730A576-E84D-4834-826F-FAEE564E6ABD}
    constexpr const GUID signal_Pattern_Prediction = { 0x4f9d0e51, 0x65e3, 0x4aaf, { 0xa3, 0x87, 0xd4, 0xd, 0xee, 0xe0, 0x72, 0x50 } }; 		// {4F9D0E51-65E3-4AAF-A387-D40DEEE07250}
   

    scgms::TSignal_Descriptor get_sig_desc();

    scgms::TFilter_Descriptor get_filter_desc();    //func to avoid static init fiasco as this is another unit than descriptor.cpp
    scgms::TModel_Descriptor get_model_desc();

   size_t Level_2_Band_Index(const double level);
   double Subclassed_Level(const double level);
}