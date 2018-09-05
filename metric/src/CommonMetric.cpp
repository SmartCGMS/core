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

#include "CommonMetric.h"

#include <algorithm>
#include <cmath>

#undef max

CCommon_Metric::CCommon_Metric(const glucose::TMetric_Parameters &params) : mParameters(params) {
	Reset();
}


HRESULT IfaceCalling CCommon_Metric::Accumulate(const double *times, const double *expected, const double *calculated, const size_t count) {

	std::vector<double> diff(count);

	//the following 3 fors are inteded for autovectorization

	for (size_t i = 0; i < count; i++)
		diff[i] = fabs(expected[i] - calculated[i]);

	if (mParameters.use_relative_error)
		for (size_t i = 0; i < count; i++)
			diff[i] /= fabs(calculated[i]);	//with wrong (e.g., MetaDE random) parameters, the result could be negative

	if (mParameters.use_squared_differences)
		for (size_t i = 0; i < count; i++)
			diff[i] *= diff[i];


	for (size_t i = 0; i < count; i++) {
		if (!std::isnan(calculated[i])) {
			TProcessed_Difference tmp;
			tmp.raw.datetime = times[i];
			tmp.raw.calculated = calculated[i];
			tmp.raw.expected = expected[i];
			tmp.difference = diff[i];

			mDifferences.push_back(tmp);			
		}
	}

	mAll_Levels_Count += count;

	return S_OK;
}


HRESULT IfaceCalling CCommon_Metric::Reset() {
	mDifferences.clear();
	mAll_Levels_Count = 0;
	return S_OK;
}


HRESULT IfaceCalling CCommon_Metric::Calculate(double *metric, size_t *levels_accumulated, size_t levels_required) {
	size_t count = mDifferences.size();

	*levels_accumulated = count;
	levels_required = std::max((decltype(levels_required))1, levels_required);

	if (/* (count<1) || */ (count < levels_required)) return S_FALSE;

	double local_metric = Do_Calculate_Metric();	//caching into the register
	
	const auto cl = std::fpclassify(local_metric);
	if ((cl != FP_NORMAL) && (cl != FP_ZERO)) return S_FALSE;

	if (mParameters.prefer_more_levels != 0) local_metric /= static_cast<double>(count);

	*metric = local_metric;

	return S_OK;
}

HRESULT IfaceCalling CCommon_Metric::Get_Parameters(glucose::TMetric_Parameters *parameters) {
	//*parameters = mParameters ; 
	memcpy(parameters, &mParameters, sizeof(mParameters));
	return S_OK;
}