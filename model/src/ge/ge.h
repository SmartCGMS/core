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

#include "..\..\..\..\common\iface\DeviceIface.h"
#include "..\..\..\..\common\rtl\FilterLib.h"

#include "..\descriptor.h"

namespace ge_model {

	/* Grammatical Evolution ISA

	State variables				
		double data[ge_model::ge_variables_count] - internal variables
	
	
	Instructions
		ibr - reads insulin bolus rate - will cover the insulin boluses as well
		cho_half - reads a half of and reduces CHO to intake by this half

		mov
		add		
		sub
		mul
		div
		pow


		ISA

		opcode	name		arg1	   arg2
		0		ibr			dst_index, dummy index
		1		cho_half	dst_index, dummy_index
		2		mov			dst_index, src_index
		3		add			dst_index, val_index
		4		sub			dst_index, val_index
		5		mul			dst_index, val_index
		6		div			dst_index, val_index
		7		pow			dst_index, exp_index

	*/
}

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CGE_Discrete_Model : public virtual glucose::CBase_Filter, public virtual glucose::IDiscrete_Model {
protected:
	double mWaiting_CHO = 0.0;		//for the cho_half instruction
	double mCurrent_IBR = 0.0;		//for the ibr instruction
	double mGE_Variables[ge_model::ge_variables_count];
	double &mOutput_Level = mGE_Variables[0];
	
	double mCurrent_Time = std::numeric_limits<double>::quiet_NaN();
	double mTime_Advance_Jitter = 0.0;			//because it will be quite difficult to ge a code that properly calculates with
												//with a dynamic time stepping => hence we evolve a code for a sufficiently small, fixed step
	const double mTime_Fixed_Stepping = 5.0*glucose::One_Minute;
	void Fixed_Step();							//runs the instrucitons, which execute the fixed step
	void Emit_Current_State();
protected:
	// glucose::CBase_Filter iface implementation
	virtual HRESULT Do_Execute(glucose::UDevice_Event event) override final;
	virtual HRESULT Do_Configure(glucose::SFilter_Configuration configuration) override final;
public:
	CGE_Discrete_Model(glucose::IModel_Parameter_Vector *parameters, glucose::IFilter *output);
	virtual ~CGE_Discrete_Model();

	// glucose::IDiscrete_Model iface
	virtual HRESULT IfaceCalling Set_Current_Time(const double new_current_time);
	virtual HRESULT IfaceCalling Step(const double time_advance_delta) override final;
};

#pragma warning( pop )
