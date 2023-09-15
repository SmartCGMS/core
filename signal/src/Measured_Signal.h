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

#include <scgms/rtl/ApproxLib.h>
#include <scgms/rtl/referencedImpl.h>

#include <map>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CMeasured_Signal : public virtual scgms::ISignal, public virtual refcnt::CReferenced {
protected:
	size_t mLast_Update_Index = 0;
    std::vector<double> mTimes;
    std::vector<double> mLevels;

    scgms::SApproximator mApprox;

public:
    CMeasured_Signal(const GUID* approx_id);
    virtual ~CMeasured_Signal() {};

    virtual HRESULT IfaceCalling Get_Discrete_Levels(double* const times, double* const levels, const size_t count, size_t* filled) const override;
    virtual HRESULT IfaceCalling Get_Discrete_Bounds(scgms::TBounds* const time_bounds, scgms::TBounds* const level_bounds, size_t* level_count) const override;
    virtual HRESULT IfaceCalling Update_Levels(const double* times, const double* levels, const size_t count) override;

    virtual HRESULT IfaceCalling Get_Continuous_Levels(scgms::IModel_Parameter_Vector* params, const double* times, double* const levels, const size_t count, const size_t derivation_order) const override;
    virtual HRESULT IfaceCalling Get_Default_Parameters(scgms::IModel_Parameter_Vector* parameters) const override;
};
#pragma warning( pop )
