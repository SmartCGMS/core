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

#include "../../../../common/iface/DeviceIface.h"
#include "../../../../common/rtl/FilterLib.h"

#include "../descriptor.h"

#include <map>
#include <vector>
#include <random>

#pragma warning( push )
#pragma warning( disable : 4250 ) // C4250 - 'class1' : inherits 'class2::member' via dominance

class CEmulated_Sensor_Filter : public virtual scgms::CBase_Filter
{
	private:
		// stored calibration record
		struct Calibration_Record
		{
			// when was the calibration issued
			double time;

			// measured value from virtual sensor needle
			double measured_value;

			// calibration value issued by user
			double calibration_value;
		};

	private:
		// random engine for noise
		std::default_random_engine mRandom_Engine;
		// distribution of noise
		std::normal_distribution<double> mDist;
		// current sensor drift
		double mSensor_Drift = 0.0;

		double mStart_Time = std::numeric_limits<double>::quiet_NaN();
		// [configurable] this much drift we add during the day (linear for now)
		double mSensor_Drift_Per_Day = 1.0;

		// [configurable] source ID (actual measured signal)
		GUID mSource_Id = Invalid_GUID;
		// [configurable] destination ID (what signal we actually pass out of filter)
		GUID mDestination_Id = Invalid_GUID;
		// [configurable] ID of calibration signal (reference signal for calibration, to calculate internal calib. model parameters)
		GUID mCalibration_Id = Invalid_GUID;
		// [configurable] noise level of measured signal
		double mNoise_Level = 0.0;
		// [configurable] minimal amount of calibration values
		size_t mMin_Calibration_Values = 2;

		// stored calibration records (possible without it, but we keep it for debugging purposes)
		std::vector<Calibration_Record> mCalibration_Diffs;

		// linear model slope
		double mCal_K = 0;
		// linear model intercept
		double mCal_Q = 0;

		// is the sensor properly calibrated?
		bool mCalibrated = false;
		// is any calibration value pending for recalculation?
		bool mPending_Calibration = false;

		// is the sensor precalibrated? i.e.; should it emit values despite not having properly calculated calibration model parameters?
		bool mPrecalibrated = false;

	protected:
		// scgms::CBase_Filter iface implementation
		virtual HRESULT Do_Execute(scgms::UDevice_Event event) override final;
		virtual HRESULT Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description) override final;

	public:
		CEmulated_Sensor_Filter(scgms::IFilter* output);
		virtual ~CEmulated_Sensor_Filter();
};

#pragma warning( pop )
