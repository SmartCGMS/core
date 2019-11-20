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

#include "Diffusion_v2_ist.h"

#include "../descriptor.h"

#include "../../../../common/rtl/SolverLib.h"

#include <cmath>
#include <algorithm>

#undef max

struct TIst_Estimate_Data {
	glucose::SSignal ist;
	const double reference_present_time;
	const double kh, h, dt;
	const double *times;
};


double present_time_objective(unsigned, const double *present_time, double *, void *estimation_data_ptr) {
	const TIst_Estimate_Data &estimation_data = *static_cast<TIst_Estimate_Data*>(estimation_data_ptr);

	const double ist_times[2] = { *present_time - estimation_data.h, *present_time };
	double ist_levels[2];
	if (estimation_data.ist->Get_Continuous_Levels(nullptr, ist_times, ist_levels, 2, glucose::apxNo_Derivation) != S_OK) return std::numeric_limits<double>::max();
	if (std::isnan(ist_levels[0]) || std::isnan(ist_levels[1])) return std::numeric_limits<double>::max();

	double estimated_present_time = *present_time + estimation_data.dt + estimation_data.kh * ist_levels[1] * (ist_levels[1] - ist_levels[0]);

	return fabs(estimated_present_time - estimation_data.reference_present_time);
};

CDiffusion_v2_ist::CDiffusion_v2_ist(glucose::WTime_Segment segment) : CDiffusion_v2_blood(segment), mBlood(segment.Get_Signal(glucose::signal_BG)) {
	if (!refcnt::Shared_Valid_All(mBlood)) throw std::exception{};
}


HRESULT IfaceCalling CDiffusion_v2_ist::Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	diffusion_v2_model::TParameters &parameters = glucose::Convert_Parameters<diffusion_v2_model::TParameters>(params, diffusion_v2_model::default_parameters);

	Eigen::Map<TVector1D> converted_times{ Map_Double_To_Eigen<TVector1D>(times, count) };
	Eigen::Map<TVector1D> converted_levels{ Map_Double_To_Eigen<TVector1D>(levels, count) };
	auto dt = Reserve_Eigen_Buffer(mDt, count);

	//into the dt vector, we put times to get blood and ist to calculate future ist aka levels at the future times
	dt = converted_times - parameters.dt;
	if ((parameters.k != 0.0) && (parameters.h != 0.0)) {
		//here comes the tricky part that the future distances will vary in the dt vector

		const double kh = parameters.k / parameters.h;
		
		//future_time = present_time + delta + kh*(present_ist - h_back_ist)
		//future_time, dt and kh are known => we need to solve this
		//current value of dt = present_time + kh*(present_ist - h_back_ist)

		//we give up the SIMD optimization due to unkown memory requirements
		for (size_t i = 0; i < count; i++) {
			//This for cycle could be paralelized with threads, but...
			//	..if called by solver, then may threads are already executing this function thus making this idea pointless - just increasing the scheduling overhead
			//	..if called just to calculate the levels, then single core is fast enough for the end user not to notice the difference

			//we need to minimize the difference between times[i] and estimate_future_time(estimated_present_time), whereas estimated_present_time we try to determine using some other algorithm
			//we could either try to (recursively with increasing detail) step through all the combinations, or try more sophisticated algoritm - e.g., NewUOA
			//we chose NewUOA as the number of instruction could be approximately the same and NewUOA is more intelligent approach than brute force search, even recursive one
			//since we no longer actually use this algorithm, we replaced NewUOA with a brute force => if ever needed more than this, we should replace the successive
			//mIst->Get_Continuous_Levels with a single call
			
			const double dt_stepping = glucose::One_Second;
			double current_dt = dt[i] - 15.0*glucose::One_Minute;
			double max_dt = dt[i] + 15.0 * glucose::One_Minute;
			double best_dt = current_dt;
			double least_error = std::numeric_limits<double>::max();

			while (current_dt <= max_dt) {

				auto calculate_current_error= [&parameters, this, kh](const double present_time, const double reference_time) {
					const double ist_times[2] = { present_time - parameters.h, present_time };
					double ist_levels[2];
					if (mIst->Get_Continuous_Levels(nullptr, ist_times, ist_levels, 2, glucose::apxNo_Derivation) != S_OK) return std::numeric_limits<double>::max();
					if (std::isnan(ist_levels[0]) || std::isnan(ist_levels[1])) return std::numeric_limits<double>::max();

					double estimated_present_time = present_time + parameters.dt + kh * ist_levels[1] * (ist_levels[1] - ist_levels[0]);

					return fabs(estimated_present_time - reference_time);
				};

				double current_error = calculate_current_error(current_dt, times[i]);
				if (current_error < least_error) {
					best_dt = current_dt;
					least_error = current_error;
				}

				current_dt += dt_stepping;
			}
			

			dt[i] = current_dt;
		}

		//by now, all dt elements should be estimated
	}

	auto present_blood = Reserve_Eigen_Buffer(mPresent_Blood,  count );
	HRESULT rc = mBlood->Get_Continuous_Levels(nullptr, dt.data(), present_blood.data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;

	auto present_ist = Reserve_Eigen_Buffer(mPresent_Ist, count );
	rc = mIst->Get_Continuous_Levels(nullptr, dt.data(), present_ist.data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;

	converted_levels =  parameters.p*present_blood
					  + parameters.cg*present_blood*(present_blood - present_ist)
					  + parameters.c;

	return S_OK;
}