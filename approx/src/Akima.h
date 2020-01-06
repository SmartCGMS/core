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

#include "../../../common/iface/ApproxIface.h"
#include "../../../common/iface/UIIface.h"
#include "../../../common/rtl/referencedImpl.h"
#include "../../../common/rtl/DeviceLib.h"


#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CAkima : public scgms::IApproximator, public virtual refcnt::CReferenced {
protected:
	scgms::WSignal mSignal;
	const size_t MINIMUM_NUMBER_POINTS = 5;
	std::vector<double> mInputTimes, mInputLevels, mCoefficients;
	bool Update();
protected:
	/**
	* Computes coefficients of Akima Interpolation.
	* Coefficients are stored in one array containing coefficients stored in
	* column fashion. Eg. first coefficients are stored between 0 and count position (inclusive).
	*/
	void Compute_Coefficients();	//modifies mCoefficients

	/**
	* From mMeasured, it computes differences within three points as needed by Akima interpolation
	*	
	*/
	double Differentiate_Three_Point_Scalar(size_t indexOfDifferentiation,
											size_t indexOfFirstSample,
											size_t indexOfSecondsample,
											size_t indexOfThirdSample);

	/**
	* Interpolate on basis of Hermite's algorithm
	*
	* @param firstDerivatives pointer to output structure with first derivates
	*/
	std::vector<double> Interpolate_Hermite_Scalar(std::vector<double> coefsOfPolynFunc);
public:
public:
	CAkima(scgms::WSignal signal, scgms::IApprox_Parameters_Vector* configuration);
	virtual ~CAkima() {};

	virtual HRESULT IfaceCalling GetLevels(const double* times, double* const levels, const size_t count, const size_t derivation_order) override;
	
};

#pragma warning( pop )