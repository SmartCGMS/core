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

#include "betapid.h"
#include "../descriptor.h"

#include <numeric>
#include <cmath>

#include <scgms/rtl/rattime.h>
#include <scgms/rtl/SolverLib.h>

CBetaPID_Insulin_Regulation::CBetaPID_Insulin_Regulation(scgms::WTime_Segment segment)
	: CCommon_Calculated_Signal(segment), mIOB(segment.Get_Signal(scgms::signal_IOB)), mIG(segment.Get_Signal(scgms::signal_IG)) {
	//
}

HRESULT CBetaPID_Insulin_Regulation::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	const betapid_insulin_regulation::TParameters &parameters = scgms::Convert_Parameters<betapid_insulin_regulation::TParameters>(params, betapid_insulin_regulation::default_parameters);

	const double h = scgms::One_Minute * 1.0;
	const size_t integral_history = 30;
	const double targetBG = 5.0;

	std::array<double, integral_history> historyTimes, historyIGs;

	std::vector<double> BGs(count);
	std::fill(BGs.begin(), BGs.end(), std::numeric_limits<double>::quiet_NaN());
	HRESULT rc = mIG->Get_Continuous_Levels(nullptr, times, BGs.data(), count, scgms::apxNo_Derivation);
	if (!Succeeded(rc)) {
		return rc;
	}

	std::vector<double> IOBs(count);
	std::fill(IOBs.begin(), IOBs.end(), 0.0);
	rc = mIOB->Get_Continuous_Levels(nullptr, times, IOBs.data(), count, scgms::apxNo_Derivation);
	if (!Succeeded(rc)) {
		return rc;
	}

	for (size_t i = 0; i < count; i++) {
		const double bt = BGs[i];
		const double iob = IOBs[i];

		// we cannot calculate insulin rate when BG/IOB is unknown at this time point
		// as we don't extrapolate (would be dangerous), wait for BG/IOB to be available
		if (std::isnan(bt) || std::isnan(IOBs[i])) {
			levels[i] = std::numeric_limits<double>::quiet_NaN();
			continue;
		}

		// error function e(t)
		const double et = targetBG - bt;

		// generate times for integral history
		std::generate(historyTimes.begin(), historyTimes.end(), [h, htime = times[i] - h*(integral_history + 1)]() mutable {
			return htime += h;
		});

		double eintegral = 0.0;
		double ederivative = 0.0;

		std::fill(historyIGs.begin(), historyIGs.end(), std::numeric_limits<double>::quiet_NaN());

		// retrieve history
		rc = mIG->Get_Continuous_Levels(nullptr, historyTimes.data(), historyIGs.data(), integral_history, scgms::apxNo_Derivation);
		if (!Succeeded(rc)) {
			return rc;
		}

		// approximate integral of error, int_0^t e(tau) dtau =~= sum_i=0^hist e(t-i*h)*h
		for (size_t j = 0; j < integral_history; j++) {
			eintegral += std::isnan(historyIGs[j]) ? 0.0 : (targetBG - historyIGs[j]); // *h --> omit the width parameter, as its constant, and would still be included in Ki parameter
		}

		rc = mIG->Get_Continuous_Levels(nullptr, &times[i], &ederivative, 1, scgms::apxFirst_Order_Derivation);
		if (Succeeded(rc)) {
			// error trend has exactly opposite sign than current BG trend
			ederivative *= -1;
		}
		else {
			return rc;
		}

		// the lower the BG is, the more the regulator conservative is
		// the regulator is "most confident" around target BG

		// error factor for derivative
		const double etf = bt / targetBG;

		// estimate BG in future, after all current IOB has been absorbed
		const double future_bg = std::max(bt - (iob / parameters.isf), 0.0);
		// estimate error factor in that future
		const double future_etf = (future_bg / targetBG);

		// multiply by ISF to obtain insulin amount needed to lower BG by certain amount; add basal insulin need to counteract to basal metabolism
		levels[i] = -parameters.isf * (parameters.kp * future_etf * et + parameters.ki * future_etf * eintegral + parameters.kd * etf * ederivative) + parameters.basal_insulin_rate;

		// as much as the regulator would want to, the dosage still needs to be non-negative (positive values would mean the BG is/would be too low)
		if (levels[i] < 0.0) {
			levels[i] = 0.0;
		}

		// TODO: move this to safety filter
		if (levels[i] > 3.5) {
			levels[i] = 3.5;
		}
	}

	return S_OK;
}

HRESULT CBetaPID_Insulin_Regulation::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {

	double *params = const_cast<double*>(betapid_insulin_regulation::default_parameters);
	return parameters->set(params, params + betapid_insulin_regulation::param_count);
}

////////////////////////////////////// BetaPID2

CBetaPID2_Insulin_Regulation::CBetaPID2_Insulin_Regulation(scgms::WTime_Segment segment)
	: CCommon_Calculated_Signal(segment), mIOB(segment.Get_Signal(scgms::signal_IOB)), mCOB(segment.Get_Signal(scgms::signal_COB)), mIG(segment.Get_Signal(scgms::signal_IG)) {
	//
}

HRESULT CBetaPID2_Insulin_Regulation::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	const betapid_insulin_regulation::TParameters &parameters = scgms::Convert_Parameters<betapid_insulin_regulation::TParameters>(params, betapid_insulin_regulation::default_parameters);

	const double h = scgms::One_Minute * 1.0;
	const size_t integral_history = 30;
	const size_t derivative_smooth_history = 5;
	const double targetBG = 6.66;

	std::array<double, integral_history> historyTimes, historyIGs;

	std::vector<double> IGs(count);
	std::fill(IGs.begin(), IGs.end(), std::numeric_limits<double>::quiet_NaN());
	HRESULT rc = mIG->Get_Continuous_Levels(nullptr, times, IGs.data(), count, scgms::apxNo_Derivation);
	if (!Succeeded(rc)) {
		return rc;
	}

	std::vector<double> IOBs(count);
	std::fill(IOBs.begin(), IOBs.end(), 0.0);
	rc = mIOB->Get_Continuous_Levels(nullptr, times, IOBs.data(), count, scgms::apxNo_Derivation);
	if (!Succeeded(rc)) {
		return rc;
	}

	std::vector<double> COBs(count);
	std::fill(COBs.begin(), COBs.end(), 0.0);
	rc = mCOB->Get_Continuous_Levels(nullptr, times, COBs.data(), count, scgms::apxNo_Derivation);
	if (!Succeeded(rc)) {
		return rc;
	}

	const auto e = [targetBG](const double v) {
		return targetBG - v;
	};

	for (size_t i = 0; i < count; i++) {

		const double bt = IGs[i];
		const double iob = IOBs[i];
		const double cob = COBs[i];

		// we cannot calculate insulin rate when BG/IOB/COB is unknown at this time point
		// as we don't extrapolate (would be dangerous), we wait for all values to be available;
		// NOTE: IOB and COB should have IG as reference signal and always emit a valid value
		//       as BetaPID also have IG as reference, these signals would come 
		if (std::isnan(bt) || std::isnan(iob) || std::isnan(cob)) {
			levels[i] = std::numeric_limits<double>::quiet_NaN();
			continue;
		}

		// error function e(t)
		const double et = e(bt);

		// generate times for integral history
		std::generate(historyTimes.begin(), historyTimes.end(), [h, htime = times[i] - h*(integral_history + 1)]() mutable {
			return htime += h;
		});

		double eintegral = 0.0;
		double ederivative = 0.0;

		std::fill(historyIGs.begin(), historyIGs.end(), std::numeric_limits<double>::quiet_NaN());

		// retrieve history
		rc = mIG->Get_Continuous_Levels(nullptr, historyTimes.data(), historyIGs.data(), integral_history, scgms::apxNo_Derivation);
		if (!Succeeded(rc)) {
			return rc;
		}

		// approximate integral of error, int_0^t e(tau) dtau =~= sum_i=0^hist e(t-i*h)*h
		for (size_t j = 0; j < integral_history; j++) {
			eintegral += std::isnan(historyIGs[j]) ? 0.0 : e(historyIGs[j]); // *h --> omit the width parameter, as its constant, and would still be included in Ki parameter
		}

		double derTimes[derivative_smooth_history];
		double valArray[derivative_smooth_history];
		for (size_t j = 1; j <= derivative_smooth_history; j++) {
			derTimes[j - 1] = times[i] - (scgms::One_Minute * 5 * (derivative_smooth_history - j));
		}

		rc = mIG->Get_Continuous_Levels(nullptr, derTimes, valArray, derivative_smooth_history, scgms::apxNo_Derivation);
		if (Succeeded(rc)) {
			bool valid = true;
			for (size_t j = 0; j < derivative_smooth_history; j++)
				valid &= !std::isnan(valArray[j]);

			if (valid)
				ederivative = (-e(valArray[derivative_smooth_history - 1]) + 8 * e(valArray[derivative_smooth_history - 2]) - 8 * e(valArray[derivative_smooth_history - 4]) + e(valArray[derivative_smooth_history - 5])) / (12 * scgms::One_Minute * 5);
		}
		else {
			return rc;
		}

		const double isf = parameters.isf;

		// the lower the BG is, the more the regulator conservative is

		// error factor for derivative
		const double etf = bt / targetBG;

		// estimate BG in future, after all current IOB and COB has been absorbed
		const double future_bg = std::max(bt - ((iob - cob*parameters.csr) / isf), 0.0);
		// estimate error factor in that future
		const double future_etf = future_bg / targetBG;

		// multiply by ISF to obtain insulin amount needed to lower BG by certain amount; add basal insulin need to counteract to basal metabolism
		levels[i] = -isf * (parameters.kp * future_etf * et + parameters.ki * future_etf * eintegral + parameters.kd * etf * ederivative) + parameters.basal_insulin_rate;

		// as much as the regulator would want to, the dosage still needs to be non-negative (positive values would mean the BG is/would be too low)
		if (levels[i] < 0.0) {
			levels[i] = 0.0;
		}

		// TODO: move this to safety filter
		if (levels[i] > 3.5) {
			levels[i] = 3.5;
		}
	}

	return S_OK;
}

HRESULT CBetaPID2_Insulin_Regulation::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(betapid_insulin_regulation::default_parameters);
	return parameters->set(params, params + betapid_insulin_regulation::param_count);
}

////////////////////////////////////// BetaPID3

CBetaPID3_Insulin_Regulation::CBetaPID3_Insulin_Regulation(scgms::WTime_Segment segment)
	: CCommon_Calculated_Signal(segment), mIOB(segment.Get_Signal(scgms::signal_IOB)), mCOB(segment.Get_Signal(scgms::signal_COB)), mISF(segment.Get_Signal(scgms::signal_Insulin_Sensitivity)), mCR(segment.Get_Signal(scgms::signal_Carb_Ratio)), mIG(segment.Get_Signal(scgms::signal_IG)) {
	//
}

HRESULT CBetaPID3_Insulin_Regulation::Get_Continuous_Levels(scgms::IModel_Parameter_Vector *params,
	const double* times, double* const levels, const size_t count, const size_t derivation_order) const {

	const betapid3_insulin_regulation::TParameters &parameters = scgms::Convert_Parameters<betapid3_insulin_regulation::TParameters>(params, betapid3_insulin_regulation::default_parameters);

	const double h = scgms::One_Minute * 5.0;
	const size_t integral_history = 12;
	const size_t derivative_smooth_history = 5;
	const double targetBG = 5.5;

	std::array<double, integral_history> historyTimes, historyBGs;

	std::vector<double> IGs(count);
	std::fill(IGs.begin(), IGs.end(), std::numeric_limits<double>::quiet_NaN());
	HRESULT rc = mIG->Get_Continuous_Levels(nullptr, times, IGs.data(), count, scgms::apxNo_Derivation);
	if (!Succeeded(rc)) {
		return rc;
	}

	std::vector<double> IOBs(count);
	std::fill(IOBs.begin(), IOBs.end(), 0.0);
	rc = mIOB->Get_Continuous_Levels(nullptr, times, IOBs.data(), count, scgms::apxNo_Derivation);
	if (!Succeeded(rc)) {
		return rc;
	}

	std::vector<double> COBs(count);
	std::fill(COBs.begin(), COBs.end(), 0.0);
	rc = mCOB->Get_Continuous_Levels(nullptr, times, COBs.data(), count, scgms::apxNo_Derivation);
	if (!Succeeded(rc)) {
		return rc;
	}

	std::vector<double> ISFs(count);
	std::fill(ISFs.begin(), ISFs.end(), 0.0);
	rc = mISF->Get_Continuous_Levels(nullptr, times, ISFs.data(), count, scgms::apxNo_Derivation);
	if (!Succeeded(rc)) {
		return rc;
	}

	std::vector<double> CRs(count);
	std::fill(CRs.begin(), CRs.end(), 0.0);
	rc = mCR->Get_Continuous_Levels(nullptr, times, CRs.data(), count, scgms::apxNo_Derivation);
	if (!Succeeded(rc)) {
		return rc;
	}

	double derTimes[derivative_smooth_history];
	double valArray[derivative_smooth_history];

	bool valid;

	const auto e = [targetBG](const double v) {
		return targetBG - v;
	};

	for (size_t i = 0; i < count; i++) {

		const double bt = IGs[i];
		const double iob = IOBs[i];
		const double cob = COBs[i];
		const double isf = ISFs[i];
		const double cr = CRs[i];

		// we cannot calculate insulin rate when BG/IOB/COB/ISF/CR is unknown at this time point
		// as we don't extrapolate (would be dangerous), we wait for all values to be available;
		// NOTE: IOB and COB should have IG as reference signal and always emit a valid value
		//       as BetaPID also have IG as reference, these signals would come 
		if (std::isnan(bt) || std::isnan(iob) || std::isnan(cob) || std::isnan(isf) || std::isnan(cr)) {
			levels[i] = std::numeric_limits<double>::quiet_NaN();
			continue;
		}

		// error function e(t) at time t
		const double et = e(bt);
		double eintegral = 0.0;
		double ederivative = 0.0;

		// calculate integral part
		{
			// generate times for integral history
			std::generate(historyTimes.begin(), historyTimes.end(), [h, htime = times[i] - h * (integral_history + 1)]() mutable {
				return htime += h;
			});

			std::fill(historyBGs.begin(), historyBGs.end(), std::numeric_limits<double>::quiet_NaN());

			// retrieve history
			rc = mIG->Get_Continuous_Levels(nullptr, historyTimes.data(), historyBGs.data(), integral_history, scgms::apxNo_Derivation);
			if (!Succeeded(rc)) return rc;

			// approximate integral of error (Riemann), int_0^t e(tau) dtau =~= sum_i=0^hist e(t-i*h)*h
			for (size_t i = 0; i < integral_history; i++) {
				// perform exponential decay before summing another value
				eintegral *= parameters.kidecay;
				// add value from history to decayed integral accumulator variable
				eintegral += std::isnan(historyBGs[i]) ? 0.0 : e(historyBGs[i]); // *h --> omit the width parameter, as its constant, and would still be included in Ki parameter
			}
		}

		// calculate derivative part
		{
			for (size_t j = 1; j <= derivative_smooth_history; j++) {
				derTimes[j - 1] = times[i] - (scgms::One_Minute * 5 * (derivative_smooth_history - j));
			}

			rc = mIG->Get_Continuous_Levels(nullptr, derTimes, valArray, derivative_smooth_history, scgms::apxNo_Derivation);
			if (Succeeded(rc)) {
				valid = true;
				// for the derivative to be valid, we need all values in smoothed history (in order to calculate Nth order accurate derivative) to be valid
				// otherwise the derivative part is considered zero => has no effect whatsoever on the control variable
				for (size_t j = 0; j < derivative_smooth_history; j++) {
					valid = valid && !std::isnan(valArray[j]);
				}

				// five-point method of derivative approximation
				if (valid) {
					ederivative = (-e(valArray[derivative_smooth_history - 1]) + 8 * e(valArray[derivative_smooth_history - 2]) - 8 * e(valArray[derivative_smooth_history - 4]) + e(valArray[derivative_smooth_history - 5])) / (12 * scgms::One_Minute * 5);
				}
			}
			else {
				return rc;
			}
		}

		// kd(t)  = b(t) / TARGET_BG
		//	- this prioritizes derivation error function when the patient is above target range

		const double etf = bt / targetBG;

		// k(t)   = (b(t) - (iob - cob*CSR) / ISF) / TARGET_BG
		//	- projected BG over the target BG - this gives the priority to proportional and integral parts based on projected BG ("in future")
		//	- projection of BG is based on current IoB and CoB - pure projection assuming stable control (!!); if this constraint is not fulfilled, the projection accuracy could not be ensured in any way
		//	- in other words - if it's just the IoB and CoB, that has the potential to reduce or increase b(t), and no other inputs disturbs the process variable, then the projection is accurate
		//	- of course this is not possible to achieve in real-world scenario, so we just assume that the projection is "accurate enough"

		// estimate BG in future, after all current IOB and COB has been absorbed
		const double future_bg = std::max(bt - ((iob - cob * cr) / isf), 0.0);
		// estimate error factor in that future
		const double future_etf = future_bg / targetBG;

		// Iex(t) = isf * k0 * ( k(t) * (PROPORTIONAL_ERR + ki0 * INTEGRAL_ERR) + kd(t) * kd0 * DERIVATIVE_OF_ERR) + BIN
		//	- standard PID control law enahnced with adaptive coefficients
		//	- the proportional error mimics healthy beta-cell function (although a few minutes delayed due to physiological constraints of insulin absorption from exogennous infusion)
		//	- the integral of error prevents wind-up by employing exponential decay and fixed integral history
		//	- the whole equation is multiplied by ISF to obtain proper response - we "convert" b(t) difference to insulin dosage, which is generally done by insulin sensitivity factor

		levels[i] = -isf * parameters.k * (future_etf * (et + parameters.ki * eintegral) + parameters.kd * etf * ederivative) + parameters.basal_insulin_rate;

		// as much as the regulator would want to, the dosage still needs to be non-negative (positive values would mean the BG is/would be too low)
		if (levels[i] < 0.0) {
			levels[i] = 0.0;
		}

		// TODO: move this to safety filter
		if (levels[i] > 3.5) {
			levels[i] = 3.5;
		}
	}

	return S_OK;
}

HRESULT CBetaPID3_Insulin_Regulation::Get_Default_Parameters(scgms::IModel_Parameter_Vector *parameters) const {
	double *params = const_cast<double*>(betapid3_insulin_regulation::default_parameters);
	return parameters->set(params, params + betapid3_insulin_regulation::param_count);
}
