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

#include "sensor.h"

#include <numeric>
#include <algorithm>
#include <cmath>

CSensor_Filter::CSensor_Filter(scgms::IFilter *output)
	: scgms::CBase_Filter(output)
{
	//
}

CSensor_Filter::~CSensor_Filter()
{
	
}

HRESULT CSensor_Filter::Do_Execute(scgms::UDevice_Event evt)
{
	// respond to measured signal value
	if (evt.signal_id() == mSource_Id)
	{
		if (std::isnan(mStart_Time))
			mStart_Time = evt.device_time();

		// the drift is constant for now, although it should be an exponential curve (the body trying to get rid of the sensor needle);
		// let's keep it linear until we implement inspection with sensor reset
		mSensor_Drift = (evt.device_time() - mStart_Time) * mSensor_Drift_Per_Day;

		// value sensed by sensor, without calibration, just "raw" value (for IG, that would be ISIG, but since we don't emulate that far, we just use raw IG with noise)
		// "drift" is a quantification of the sensor "numbness" due to immunity response of the body itself
		const double sensedValue = evt.level() + mDist(mRandom_Engine) + mSensor_Drift;

		// if there is a new calibration value pending
		if (mPending_Calibration)
		{
			for (auto& cal : mCalibration_Diffs)
			{
				if (std::isnan(cal.measured_value))
					cal.measured_value = sensedValue;
			}

			mPending_Calibration = false;

			// is it possible to calibrate?
			if (mCalibration_Diffs.size() >= mMin_Calibration_Values)
			{
				if (mCalibration_Diffs.size() == 1)
				{
					// with just a single value, let us shift the curve without slope
					// this may lead to a significant "jump" in measured signal, however this may sometimes be wanted (e.g.; when the signal is steady enough to do this
					// and the outer code is aware of that)
					// in reality, we often require sensor pre-calibration with at least 3-4 calibrations once the sensor is applied
					mCal_K = 0.0;
					mCal_Q = mCalibration_Diffs[0].measured_value - mCalibration_Diffs[0].calibration_value;
				}
				else
				{
					// cache the calibrations to vectors
					std::vector<double> times(mCalibration_Diffs.size());
					std::vector<double> values(mCalibration_Diffs.size());

					// fill the vectors
					for (size_t i = 0; i < mCalibration_Diffs.size(); i++)
					{
						times[i] = mCalibration_Diffs[i].time - mStart_Time;
						values[i] = mCalibration_Diffs[i].measured_value - mCalibration_Diffs[i].calibration_value;
					}

					// calculate means
					double meanTime = std::accumulate(times.begin(), times.end(), 0.0) / static_cast<double>(times.size());
					double meanValue = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());

					// calculate linear regression coefficients
					double b1_a = 0.0;
					double b1_b = 0.0;
					for (size_t i = 0; i < times.size(); i++)
					{
						b1_a += (values[i] - meanValue) * (times[i] - meanTime);
						b1_b += (values[i] - meanValue) * (values[i] - meanValue);
					}

					// retrieve slope and intercept
					mCal_K = b1_a / b1_b;
					mCal_Q = meanTime - mCal_K * meanValue;
				}

				mCalibrated = true;
			}
		}

		// if the sensor is calibrated, or is precalibrated (i.e.; the user knows what he's doing), emit measured value (-> mapping with noise and calibration)
		if (mCalibrated || mPrecalibrated)
		{
			scgms::UDevice_Event evt2{ scgms::NDevice_Event_Code::Level };
			evt2.device_time() = evt.device_time();
			evt2.level() = sensedValue - (mCal_K * (evt.device_time() - mStart_Time) + mCal_Q); // subtract the calibration
			evt2.signal_id() = mDestination_Id;
			evt2.device_id() = sensor::filter_id;
			evt2.segment_id() = evt.segment_id();

			mOutput.Send(evt2);
		}
	}
	// respond to calibration signal value
	else if (evt.signal_id() == mCalibration_Id)
	{
		// push it to calibration vector and signalize calibration
		// the calibration will be issued on next measured value arrival

		mCalibration_Diffs.push_back({ evt.device_time(), std::numeric_limits<double>::quiet_NaN(), evt.level() });
		mPending_Calibration = true;
	}

	return mOutput.Send(evt);
}


HRESULT CSensor_Filter::Do_Configure(scgms::SFilter_Configuration configuration, refcnt::Swstr_list& error_description)
{
	mSource_Id = configuration.Read_GUID(rsSignal_Source_Id, mSource_Id);
	mDestination_Id = configuration.Read_GUID(rsSignal_Destination_Id, mDestination_Id);
	mCalibration_Id = configuration.Read_GUID(sensor::rsCalibration_Signal_Id, mCalibration_Id);
	mPrecalibrated = configuration.Read_Bool(sensor::rsPrecalibrated, mPrecalibrated);
	mNoise_Level = configuration.Read_Double(sensor::rsNoise_Level, mNoise_Level);
	int64_t cal_vals = configuration.Read_Int(sensor::rsCalibration_Min_Value_Count, mMin_Calibration_Values);
	mSensor_Drift_Per_Day = configuration.Read_Double(sensor::rsSensor_Drift_Per_Day, mSensor_Drift_Per_Day);

	if (mNoise_Level < 0)
	{
		error_description.push(L"Sensor noise level must be non-negative");
		return E_INVALIDARG;
	}

	if (cal_vals < 1)
	{
		error_description.push(L"Sensor needs to be calibrated with at least 1 value");
		return E_INVALIDARG;
	}

	mMin_Calibration_Values = static_cast<size_t>(cal_vals);

	mRandom_Engine = std::default_random_engine{ std::random_device{}() };
	mDist = std::normal_distribution<double>{ 0, mNoise_Level };

	return S_OK;
}
