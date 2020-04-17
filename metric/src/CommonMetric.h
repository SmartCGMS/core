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

#include "../../../common/iface/SolverIface.h"
#include "../../../common/rtl/referencedImpl.h"

#include <vector>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

struct TDifference_Point {
	double expected;
	double calculated;
	double datetime;		//datetime when the level was measured
};

struct TProcessed_Difference {
	TDifference_Point raw;
	double difference;
};

class CCommon_Metric : public virtual scgms::IMetric, public virtual refcnt::CReferenced {
protected:
	const scgms::TMetric_Parameters mParameters;
	std::vector<TProcessed_Difference> mDifferences;
protected:
	virtual double Do_Calculate_Metric() = 0;
	//Particular metrics should only override this method and no else method
public:
	CCommon_Metric(const scgms::TMetric_Parameters& params);
	virtual ~CCommon_Metric() {};

	virtual HRESULT IfaceCalling Accumulate(const double *times, const double *reference, const double *calculated, const size_t count) final;
	virtual HRESULT IfaceCalling Reset() final;
	virtual HRESULT IfaceCalling Calculate(double *metric, size_t *levels_accumulated, size_t levels_required) final;
	virtual HRESULT IfaceCalling Get_Parameters(scgms::TMetric_Parameters *parameters) final;
};

#pragma warning( pop )
