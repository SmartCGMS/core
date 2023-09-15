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

#include "../../../../common/iface/ApproxIface.h"
#include "../../../../common/iface/UIIface.h"
#include "../../../../common/rtl/referencedImpl.h"
#include "../../../../common/rtl/DeviceLib.h"

#include <mutex>

#include "AvgElementaryFunctions.h"



struct TAvgExpApproximationParams {
	size_t Passes;
	size_t Iterations;
	size_t EpsilonType;
	double Epsilon;
	double ResamplingStepping;
} ;

typedef struct {
	size_t ApproximationMethod;	// = apxmAverageExponential 
	union {
		TAvgExpApproximationParams avgexp;
	};
} TApproximationParams;


extern const TApproximationParams dfApproximationParams;

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance


class CAvgExpApprox : public scgms::IApproximator, public virtual refcnt::CReferenced {
protected:
protected:
	scgms::WSignal mSignal;

	std::mutex mUpdate_Guard;
	bool Update();
protected:	
	const TApproximationParams mParameters = dfApproximationParams;
	TAvgExpVector vmPoints;
	bool GetLeftPoint(double desiredtime, size_t *index) const;
		//for X returns index of point that is its direct left, aka preceding, neighbor
		//returns false if not found

protected:
	const TAvgElementary mAvgElementary;
public:
	CAvgExpApprox(scgms::WSignal signal, const TAvgElementary &avgelementary);
	virtual ~CAvgExpApprox();

	TAvgExpVector* getPoints();
	HRESULT GetLevels_Internal(const double* times, double* const levels, const size_t count, const size_t derivation_order);

	virtual HRESULT IfaceCalling GetLevels(const double* times, double* const levels, const size_t count, const size_t derivation_order) override;
};

#pragma warning( pop )


