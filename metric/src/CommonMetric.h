/**
 * SmartCGMS - continuous glucose monitoring and controlling framework
 * https://diabetes.zcu.cz/
 *
 * Contact:
 * diabetes@mail.kiv.zcu.cz
 * Medical Informatics, Department of Computer Science and Engineering
 * Faculty of Applied Sciences, University of West Bohemia
 * Technicka 8
 * 314 06, Pilsen
 *
 * Licensing terms:
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * a) For non-profit, academic research, this software is available under the
 *    GPLv3 license. When publishing any related work, user of this software
 *    must:
 *    1) let us know about the publication,
 *    2) acknowledge this software and respective literature - see the
 *       https://diabetes.zcu.cz/about#publications,
 *    3) At least, the user of this software must cite the following paper:
 *       Parallel software architecture for the next generation of glucose
 *       monitoring, Proceedings of the 8th International Conference on Current
 *       and Future Trends of Information and Communication Technologies
 *       in Healthcare (ICTH 2018) November 5-8, 2018, Leuven, Belgium
 * b) For any other use, especially commercial use, you must contact us and
 *    obtain specific terms and conditions for the use of the software.
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

class CCommon_Metric : public virtual glucose::IMetric, public virtual refcnt::CReferenced {
protected:
	size_t mAll_Levels_Count = 0;
	const glucose::TMetric_Parameters mParameters;
	std::vector<TProcessed_Difference> mDifferences;	
protected:
	virtual double Do_Calculate_Metric() = 0;
	//Particular metrics should only override this method and no else method
public:
	CCommon_Metric(const glucose::TMetric_Parameters &params);
	virtual ~CCommon_Metric() {};

	virtual HRESULT IfaceCalling Accumulate(const double *times, const double *reference, const double *calculated, const size_t count) final;
	virtual HRESULT IfaceCalling Reset() final;
	virtual HRESULT IfaceCalling Calculate(double *metric, size_t *levels_accumulated, size_t levels_required) final;
	virtual HRESULT IfaceCalling Get_Parameters(glucose::TMetric_Parameters *parameters) final;
};

#pragma warning( pop )