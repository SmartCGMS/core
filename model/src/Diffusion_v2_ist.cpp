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

#include "descriptor.h"

#include <cmath>
#include <nlopt.hpp>

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
	mSource_Signal = segment.Get_Signal(glucose::signal_IG);
	if (!refcnt::Shared_Valid_All(mBlood, mSource_Signal)) throw std::exception{};
}


HRESULT IfaceCalling CDiffusion_v2_ist::Get_Continuous_Levels(glucose::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	diffusion_v2_model::TParameters &parameters = Convert_Parameters<diffusion_v2_model::TParameters>(params, diffusion_v2_model::default_parameters);

	Eigen::Map<TVector1D> converted_times{ Map_Double_To_Eigen<TVector1D>(times, count) };
	Eigen::Map<TVector1D> converted_levels{ Map_Double_To_Eigen<TVector1D>(levels, count) };
	CPooled_Buffer<TVector1D> dt = mVector1D_Pool.pop( count );

	//into the dt vector, we put times to get blood and ist to calculate future ist aka levels at the future times
	dt.element() = converted_times - parameters.dt;
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

			double minf;
			std::vector<double> estimated_present_time(1);
			TIst_Estimate_Data estimatation_data{ mIst, times[i], kh, parameters.h, parameters.dt, times };

			// we need initial estimate to fit lower and upper bounds passed to nlopt
			estimated_present_time[0] = dt.element()[i];

			nlopt::opt opt(nlopt::LN_BOBYQA, 1); //just one double - NewUOA requires at least 2 parameters, may be Simplex/Nelder-Mead would do the job as well?
			opt.set_min_objective(present_time_objective, &estimatation_data);

			opt.set_lower_bounds(dt.element()[i] - glucose::One_Hour);
			opt.set_upper_bounds(dt.element()[i] + glucose::One_Hour);
			opt.set_xtol_rel(0.1*glucose::One_Second);
			try
			{
				/*nlopt::result result = */opt.optimize(estimated_present_time, minf);
			}
			catch (nlopt::roundoff_limited&)
			{
				//
			}

			dt.element()[i] = estimated_present_time[0];
		}

		//by now, all dt elements should be estimated
	}

	CPooled_Buffer<TVector1D> present_blood = mVector1D_Pool.pop(count );
	HRESULT rc = mBlood->Get_Continuous_Levels(nullptr, dt.element().data(), present_blood.element().data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;

	CPooled_Buffer<TVector1D> present_ist = mVector1D_Pool.pop ( count );
	rc = mIst->Get_Continuous_Levels(nullptr, dt.element().data(), present_ist.element().data(), count, glucose::apxNo_Derivation);
	if (rc != S_OK) return rc;

	converted_levels =  parameters.p*present_blood.element()
					  + parameters.cg*present_blood.element()*(present_blood.element() - present_ist.element())
					  + parameters.c;

	return S_OK;
}