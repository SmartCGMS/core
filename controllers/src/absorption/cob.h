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

#include "../../../../common/rtl/Common_Calculated_Signal.h"

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CCarbohydrates_On_Board : public CCommon_Calculated_Signal {

	private:
		double Calculate_Total_COB(double nowTime, double peak, double dia) const;
		virtual double Calculate_Signal(double bolusTime, double bolusValue, double nowTime, double peak, double dia) const = 0;

	protected:
		scgms::SSignal mSource_Signal;

	public:
		CCarbohydrates_On_Board(scgms::WTime_Segment segment);
		virtual ~CCarbohydrates_On_Board() {};

		//scgms::ISignal iface
		virtual HRESULT IfaceCalling Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
			const double* times, double* const levels, const size_t count, const size_t derivation_order) const override;
		virtual HRESULT IfaceCalling Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const override;
};

class CCarbohydrates_On_Board_Bilinear : public CCarbohydrates_On_Board {

	private:
		virtual double Calculate_Signal(double bolusTime, double bolusValue, double nowTime, double peak, double dia) const override;

	public:
		CCarbohydrates_On_Board_Bilinear(scgms::WTime_Segment segment) : CCarbohydrates_On_Board(segment) {};
};

#pragma warning( pop )
