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

#include <cstdint>

/*
 * Substitutional tiny implementation for std::optional / boost::optional, since this project
 * could not be compiled with C++17 standard due to SimpleINI and xlnt incompatibility
 * NOTE: this is definitelly not complete implementation, it just implements what we truly need
 *       within extraction
 */
template<typename T>
class COptional
{
	private:
		T mVal;
		bool mHasValue = false;

	public:
		T & operator=(T const& val) { mVal = val; mHasValue = true; return mVal; }
		auto operator<(T const& val) const { return mVal < val; }
		auto operator>(T const& val) const { return mVal > val; }
		auto operator<=(T const& val) const { return mVal <= val; }
		auto operator>=(T const& val) const { return mVal >= val; }
		auto operator==(T const& val) const { return mVal == val; }

		T value() const { return mVal; }

		void reset() { mHasValue = false; }
		bool has_value() const { return mHasValue; }
};

/*
 * Structure for holding one record from source file
 * This does not contain any more columns since we don't need to hold i.e. segment ID because
 * the extraction is just temporary
 */
struct CMeasured_Value
{
	// julian date of measured date/time
	double mMeasuredAt;
	// BG value
	COptional<double> mBlood;
	// IG value
	COptional<double> mIst;
	// ISIG value
	COptional<double> mIsig;
	// insulin bolus amount
	COptional<double> mInsulinBolus;
	// insulin basal rate
	COptional<double> mInsulinBasalRate;
	// insulin temp basal rate (gets cancelled on end, original value is set back to pump)
	COptional<double> mInsulinTempBasalRate;
	// timestamp of temp basal rate end (always set when mInsulinTempBasalRate is valid)
	COptional<double> mInsulinTempBasalRateEnd;
	// carbohydrates amount
	COptional<double> mCarbohydrates;
	// BG sensor calibration value
	COptional<double> mCalibration;
	// physical activity/excercise intensity
	COptional<double> mPhysicalActivity;
	// physical activity/excercise duration
	COptional<double> mPhysicalActivityDuration;
	// subject skin temperature
	COptional<double> mSkinTemperature;
	// subject sorrounding air temperature
	COptional<double> mAirTemperature;
	// subject heart rate value
	COptional<double> mHeartrate;
	// subject electrodermal activity
	COptional<double> mElectrodermalActivity;
	// step count
	COptional<double> mSteps;
	// sleep quality
	COptional<double> mSleepQuality;
	// sleep end
	COptional<double> mSleepEnd;
	// acceleration vector magnitude
	COptional<double> mAccelerationMagnitude;
	// optional information
	COptional<std::string> mStringNote;

	// reset optional fields to allow reuse
	void reuse()
	{
		mBlood.reset();
		mIst.reset();
		mIsig.reset();
		mInsulinBolus.reset();
		mInsulinBasalRate.reset();
		mInsulinTempBasalRate.reset();
		mInsulinTempBasalRateEnd.reset();
		mCarbohydrates.reset();
		mCalibration.reset();
		mPhysicalActivity.reset();
		mPhysicalActivityDuration.reset();
		mSkinTemperature.reset();
		mAirTemperature.reset();
		mHeartrate.reset();
		mElectrodermalActivity.reset();
		mSteps.reset();
		mSleepQuality.reset();
		mSleepEnd.reset();
		mAccelerationMagnitude.reset();
		mStringNote.reset();
	}
};
